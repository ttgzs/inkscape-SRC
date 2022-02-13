// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Page aux toolbar: Temp until we convert all toolbars to ui files with Gio::Actions.
 */
/* Authors:
 *   Martin Owens <doctormo@geek-2.com>

 * Copyright (C) 2021 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "page-toolbar.h"

#include <glibmm/i18n.h>
#include <gtkmm.h>
#include <regex>

#include "desktop.h"
#include "document-undo.h"
#include "document.h"
#include "io/resource.h"
#include "object/sp-namedview.h"
#include "object/sp-page.h"
#include "ui/icon-names.h"
#include "ui/tools/pages-tool.h"
#include "util/paper.h"
#include "util/units.h"

using Inkscape::IO::Resource::UIS;

namespace Inkscape {
namespace UI {
namespace Toolbar {

PageToolbar::PageToolbar(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &builder, SPDesktop *desktop)
    : Gtk::Toolbar(cobject)
    , _desktop(desktop)
    , combo_page_sizes(nullptr)
    , text_page_label(nullptr)
{
    builder->get_widget("page_sizes", combo_page_sizes);
    builder->get_widget("page_label", text_page_label);
    builder->get_widget("page_pos", label_page_pos);
    builder->get_widget("page_backward", btn_page_backward);
    builder->get_widget("page_foreward", btn_page_foreward);
    builder->get_widget("page_delete", btn_page_delete);
    builder->get_widget("page_move_objects", btn_move_toggle);
    builder->get_widget("sep1", sep1);

    if (text_page_label) {
        text_page_label->signal_changed().connect(sigc::mem_fun(*this, &PageToolbar::labelEdited));
    }

    if (combo_page_sizes) {
        combo_page_sizes->signal_changed().connect(sigc::mem_fun(*this, &PageToolbar::sizeChoose));
        entry_page_sizes = dynamic_cast<Gtk::Entry *>(combo_page_sizes->get_child());
        if (entry_page_sizes) {
            entry_page_sizes->signal_activate().connect(sigc::mem_fun(*this, &PageToolbar::sizeChanged));
        }
        auto& page_sizes = Inkscape::PaperSize::getPageSizes();
        for (int i = 0; i < page_sizes.size(); i++) {
            combo_page_sizes->append(std::to_string(i), page_sizes[i].getDescription());
        }
    }

    // Watch for when the tool changes
    _ec_connection = _desktop->connectEventContextChanged(sigc::mem_fun(*this, &PageToolbar::toolChanged));

    // Constructed by a builder, so we're going to protect the widget from destruction.
    this->reference();
    was_referenced = true;
}

void PageToolbar::on_parent_changed(Gtk::Widget *)
{
    if (was_referenced) {
        // Undo the gtkbuilder protection now that we have a parent
        this->unreference();
        was_referenced = false;
    }
}

PageToolbar::~PageToolbar()
{
    _ec_connection.disconnect();
    toolChanged(nullptr, nullptr);
}

void PageToolbar::toolChanged(SPDesktop *desktop, Inkscape::UI::Tools::ToolBase *ec)
{
    // Disconnect previous page changed signal
    _page_selected.disconnect();
    _pages_changed.disconnect();
    _page_modified.disconnect();
    _document = nullptr;

    if (dynamic_cast<Inkscape::UI::Tools::PagesTool *>(ec)) {
        // Save the document and page_manager for future use.
        if ((_document = desktop->getDocument())) {
            auto &page_manager = _document->getPageManager();
            // Connect the page changed signal and indicate changed
            _pages_changed = page_manager.connectPagesChanged(sigc::mem_fun(*this, &PageToolbar::pagesChanged));
            _page_selected = page_manager.connectPageSelected(sigc::mem_fun(*this, &PageToolbar::selectionChanged));
            // Update everything now.
            pagesChanged();
        }
    }
}

void PageToolbar::labelEdited()
{
    auto text = text_page_label->get_text();
    if (auto page = _document->getPageManager().getSelected()) {
        page->setLabel(text.empty() ? nullptr : text.c_str());
        DocumentUndo::maybeDone(_document, "page-relabel", _("Relabel Page"), INKSCAPE_ICON("tool-pages"));
    }
}

void PageToolbar::sizeChoose()
{
    try {
        auto page_id = std::stoi(combo_page_sizes->get_active_id());
        auto& page_sizes = Inkscape::PaperSize::getPageSizes();
        if (page_id >= 0 && page_id < page_sizes.size()) {
            auto&& ps = page_sizes[page_id];
            auto smaller = ps.unit->convert(ps.smaller, "px");
            auto larger = ps.unit->convert(ps.larger, "px");
            _document->getPageManager().resizePage(smaller, larger);
            DocumentUndo::maybeDone(_document, "page-resize", _("Resize Page"), INKSCAPE_ICON("tool-pages"));
        }
    } catch (std::invalid_argument const &e) {
        // Ignore because user is typing into Entry
    }
}

double PageToolbar::_unit_to_size(std::string number, std::string unit_str, std::string backup)
{
    // We always support comma, even if not in that particular locale.
    std::replace(number.begin(), number.end(), ',', '.');
    double value = std::stod(number);

    // Get the best unit, for example 50x40cm means cm for both
    auto unit = _document->getDisplayUnit();
    if (unit_str.empty() && !backup.empty())
        unit_str = backup;
    if (unit_str == "\"")
        unit_str = "in";

    // Convert from user entered unit to display unit
    if (!unit_str.empty())
        return Inkscape::Util::Quantity::convert(value, unit_str, unit);

    // Default unit
    return value;
}

/**
 * A manually typed input size, parse out what we can understand from
 * the text or ignore it if the text can't be parsed.
 *
 * Format: 50cm x 40mm
 *         20',40"
 *         30,4-40.2
 */
void PageToolbar::sizeChanged()
{
    // Parse the size out of the typed text if possible.
    auto text = std::string(combo_page_sizes->get_active_text());
    // This does not support negative values, because pages can not be negatively sized.
    static std::string arg = "([\\d,\\.]+)(px|mm|cm|in|\\\")?";
    // We can't support × here since it's UTF8 and this doesn't match
    static std::regex re_size("^ *" + arg + " ?([Xx,\\-]) ?" + arg + " *$");

    std::smatch matches;
    if (std::regex_match(text, matches, re_size)) {
        double width = _unit_to_size(matches[1], matches[2], matches[5]);
        double height = _unit_to_size(matches[4], matches[5], matches[2]);
        if (width > 0 && height > 0) {
            auto scale = _document->getDocumentScale()[0];
            _document->getPageManager().resizePage(width * scale, height * scale);
        }
    }
    setSizeText(_document->getPageManager().getSelected());
}

/**
 * Sets the size of the current page into the entry page size.
 */
void PageToolbar::setSizeText(SPPage *page)
{
    auto unit = _document->getDisplayUnit();
    double width = _document->getWidth().value(unit);
    double height = _document->getHeight().value(unit);
    if (page) {
        auto rect = page->getRect();
        width = rect.width();
        height = rect.height();
    }
    entry_page_sizes->set_placeholder_text(_("ex.: 100x100cm"));
    entry_page_sizes->set_tooltip_text(_("Type in width & height of a page. (ex.: 100x100cm, 10cmx100mm)\n"
                                        "or choose preset from dropdown."));
    if (auto page_size = Inkscape::PaperSize::findPaperSize(width, height, unit)) {
        entry_page_sizes->set_text(page_size->getDescription());
    } else {
        entry_page_sizes->set_text(Inkscape::PaperSize::toDescription(_("Custom"), width, height, unit));
    }
}

void PageToolbar::pagesChanged()
{
    selectionChanged(_document->getPageManager().getSelected());
}

void PageToolbar::selectionChanged(SPPage *page)
{
    _page_modified.disconnect();
    auto &page_manager = _document->getPageManager();
    text_page_label->set_tooltip_text(_("Page label"));

    // Set label widget content with page label.
    if (page) {
        text_page_label->set_sensitive(true);
        text_page_label->set_placeholder_text(page->getDefaultLabel());

        if (auto label = page->label()) {
            text_page_label->set_text(label);
        } else {
            text_page_label->set_text("");
        }

        // Set the position label
        gchar *pos = g_strdup_printf(_("%d/%d"), page->getPagePosition(), page_manager.getPageCount());
        label_page_pos->set_label(pos);
        g_free(pos);

        _page_modified = page->connectModified([=](SPObject *obj, unsigned int /*flags*/) {
            if (auto page = dynamic_cast<SPPage *>(obj)) {
                selectionChanged(page);
            }
        });
    } else {
        text_page_label->set_text("");
        text_page_label->set_sensitive(false);
        text_page_label->set_placeholder_text(_("Single Page Document"));
        label_page_pos->set_label("-");
    }
    if (!page_manager.hasPrevPage() && !page_manager.hasNextPage() && !page) {
        sep1->set_visible(false);
        label_page_pos->get_parent()->set_visible(false);
        btn_page_backward->set_visible(false);
        btn_page_foreward->set_visible(false);
        btn_page_delete->set_visible(false);
        btn_move_toggle->set_sensitive(false);
    } else {
        // Set the forward and backward button sensitivities
        sep1->set_visible(true);
        label_page_pos->get_parent()->set_visible(true);
        btn_page_backward->set_visible(true);
        btn_page_foreward->set_visible(true);
        btn_page_backward->set_sensitive(page_manager.hasPrevPage());
        btn_page_foreward->set_sensitive(page_manager.hasNextPage());
        btn_page_delete->set_visible(true);
        btn_move_toggle->set_sensitive(true);
    }
    setSizeText(page);
}

GtkWidget *PageToolbar::create(SPDesktop *desktop)
{
    Glib::ustring page_toolbar_builder_file = get_filename(UIS, "toolbar-page.ui");
    PageToolbar *toolbar = nullptr;

    try {
        auto builder = Gtk::Builder::create_from_file(page_toolbar_builder_file);
        builder->get_widget_derived("page-toolbar", toolbar, desktop);

        if (!toolbar) {
            std::cerr << "InkscapeWindow: Failed to load page toolbar!" << std::endl;
            return nullptr;
        }
        // Usually we should be packing this widget into a parent before the builder
        // is destroyed, but the create method expects a blind widget so this widget
        // contains a special keep-alive pattern which can be removed when refactoring.
    } catch (const Glib::Error &ex) {
        std::cerr << "PageToolbar: " << page_toolbar_builder_file << " file not read! " << ex.what() << std::endl;
    }
    return GTK_WIDGET(toolbar->gobj());
}


} // namespace Toolbar
} // namespace UI
} // namespace Inkscape

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
