
/* ###################################################
## This file generated by quotefile.pl from
## inkscape_py.py on Thu Dec  2 14:51:28 2004
## DO NOT EDIT
################################################### */

static char *inkscape_module_script =
"# This file was created automatically by SWIG.\n"
"# Don't modify this file, modify the SWIG interface instead.\n"
"# This file is compatible with both classic and new-style classes.\n"
"\n"
"import _inkscape_py\n"
"\n"
"def _swig_setattr_nondynamic(self,class_type,name,value,static=1):\n"
"    if (name == \"this\"):\n"
"        if isinstance(value, class_type):\n"
"            self.__dict__[name] = value.this\n"
"            if hasattr(value,\"thisown\"): self.__dict__[\"thisown\"] = value.thisown\n"
"            del value.thisown\n"
"            return\n"
"    method = class_type.__swig_setmethods__.get(name,None)\n"
"    if method: return method(self,value)\n"
"    if (not static) or hasattr(self,name) or (name == \"thisown\"):\n"
"        self.__dict__[name] = value\n"
"    else:\n"
"        raise AttributeError(\"You cannot add attributes to %s\" % self)\n"
"\n"
"def _swig_setattr(self,class_type,name,value):\n"
"    return _swig_setattr_nondynamic(self,class_type,name,value,0)\n"
"\n"
"def _swig_getattr(self,class_type,name):\n"
"    method = class_type.__swig_getmethods__.get(name,None)\n"
"    if method: return method(self)\n"
"    raise AttributeError,name\n"
"\n"
"import types\n"
"try:\n"
"    _object = types.ObjectType\n"
"    _newclass = 1\n"
"except AttributeError:\n"
"    class _object : pass\n"
"    _newclass = 0\n"
"del types\n"
"\n"
"\n"
"\n"
"getInkscape = _inkscape_py.getInkscape\n"
"class Inkscape(_object):\n"
"    __swig_setmethods__ = {}\n"
"    __setattr__ = lambda self, name, value: _swig_setattr(self, Inkscape, name, value)\n"
"    __swig_getmethods__ = {}\n"
"    __getattr__ = lambda self, name: _swig_getattr(self, Inkscape, name)\n"
"    def __init__(self): raise RuntimeError, \"No constructor defined\"\n"
"    def __repr__(self):\n"
"        return \"<%s.%s; proxy of C++ Inkscape::Extension::Script::Inkscape instance at %s>\" % (self.__class__.__module__, self.__class__.__name__, self.this,)\n"
"    def __del__(self, destroy=_inkscape_py.delete_Inkscape):\n"
"        try:\n"
"            if self.thisown: destroy(self)\n"
"        except: pass\n"
"\n"
"    def getDesktop(*args): return _inkscape_py.Inkscape_getDesktop(*args)\n"
"\n"
"class InkscapePtr(Inkscape):\n"
"    def __init__(self, this):\n"
"        _swig_setattr(self, Inkscape, 'this', this)\n"
"        if not hasattr(self,\"thisown\"): _swig_setattr(self, Inkscape, 'thisown', 0)\n"
"        _swig_setattr(self, Inkscape,self.__class__,Inkscape)\n"
"_inkscape_py.Inkscape_swigregister(InkscapePtr)\n"
"\n"
"class Desktop(_object):\n"
"    __swig_setmethods__ = {}\n"
"    __setattr__ = lambda self, name, value: _swig_setattr(self, Desktop, name, value)\n"
"    __swig_getmethods__ = {}\n"
"    __getattr__ = lambda self, name: _swig_getattr(self, Desktop, name)\n"
"    def __init__(self): raise RuntimeError, \"No constructor defined\"\n"
"    def __repr__(self):\n"
"        return \"<%s.%s; proxy of C++ Inkscape::Extension::Script::Desktop instance at %s>\" % (self.__class__.__module__, self.__class__.__name__, self.this,)\n"
"    def __del__(self, destroy=_inkscape_py.delete_Desktop):\n"
"        try:\n"
"            if self.thisown: destroy(self)\n"
"        except: pass\n"
"\n"
"    def getDocument(*args): return _inkscape_py.Desktop_getDocument(*args)\n"
"\n"
"class DesktopPtr(Desktop):\n"
"    def __init__(self, this):\n"
"        _swig_setattr(self, Desktop, 'this', this)\n"
"        if not hasattr(self,\"thisown\"): _swig_setattr(self, Desktop, 'thisown', 0)\n"
"        _swig_setattr(self, Desktop,self.__class__,Desktop)\n"
"_inkscape_py.Desktop_swigregister(DesktopPtr)\n"
"\n"
"class Document(_object):\n"
"    __swig_setmethods__ = {}\n"
"    __setattr__ = lambda self, name, value: _swig_setattr(self, Document, name, value)\n"
"    __swig_getmethods__ = {}\n"
"    __getattr__ = lambda self, name: _swig_getattr(self, Document, name)\n"
"    def __init__(self): raise RuntimeError, \"No constructor defined\"\n"
"    def __repr__(self):\n"
"        return \"<%s.%s; proxy of C++ Inkscape::Extension::Script::Document instance at %s>\" % (self.__class__.__module__, self.__class__.__name__, self.this,)\n"
"    def __del__(self, destroy=_inkscape_py.delete_Document):\n"
"        try:\n"
"            if self.thisown: destroy(self)\n"
"        except: pass\n"
"\n"
"    def hello(*args): return _inkscape_py.Document_hello(*args)\n"
"\n"
"class DocumentPtr(Document):\n"
"    def __init__(self, this):\n"
"        _swig_setattr(self, Document, 'this', this)\n"
"        if not hasattr(self,\"thisown\"): _swig_setattr(self, Document, 'thisown', 0)\n"
"        _swig_setattr(self, Document,self.__class__,Document)\n"
"_inkscape_py.Document_swigregister(DocumentPtr)\n"
"\n"
"\n"
"";
