tools_dir = $(top_srcdir)/tools

AM_CFLAGS = \
    -I$(top_srcdir) \
    -I$(top_builddir) \
    $(WARN_CFLAGS) \
    $(LIBEMPATHY_CFLAGS)

EXTRA_DIST = \
    all.xml \
    misc.xml \
    Channel_Handler.xml \
    Stream_Engine.xml

noinst_LTLIBRARIES = libemp-extensions.la

libemp_extensions_la_LIBADD = \
    $(TELEPATHY_GLIB_LIBS)

# The client-specific parts are built into a separate .o file, so the linker
# can discard them when linking services. The service-specific parts are
# in svc-*.c, so we don't need an extensions-svc.c.
libemp_extensions_la_SOURCES = \
    extensions.c \
    extensions-cli.c \
    extensions.h

nodist_libemp_extensions_la_SOURCES = \
    _gen/signals-marshal.c \
    _gen/signals-marshal.h \
    _gen/signals-marshal.list \
    _gen/register-dbus-glib-marshallers-body.h \
    _gen/enums.h \
    _gen/gtypes.h \
    _gen/gtypes-body.h \
    _gen/interfaces.h \
    _gen/interfaces-body.h \
    _gen/cli-misc.h \
    _gen/cli-misc-body.h \
    _gen/svc-misc.h \
    _gen/svc-misc.c

BUILT_SOURCES = \
    _gen/all.xml \
    _gen/misc.xml \
    $(nodist_libemp_extensions_la_SOURCES) \
    extensions.html

CLEANFILES = $(BUILT_SOURCES)

XSLTPROCFLAGS = --nonet --novalid

# Generated files which can be generated for all categories simultaneously

_gen/%.xml: %.xml $(wildcard *.xml)
	$(mkdir_p) _gen
	$(XSLTPROC) $(XSLTPROCFLAGS) --xinclude $(tools_dir)/identity.xsl \
		$< > $@

extensions.html: _gen/all.xml $(tools_dir)/doc-generator.xsl
	$(XSLTPROC) $(XSLTPROCFLAGS) \
		$(tools_dir)/doc-generator.xsl \
		$< > $@

_gen/gtypes.h _gen/gtypes-body.h: _gen/all.xml \
	$(top_srcdir)/tools/glib-gtypes-generator.py
	$(PYTHON) $(top_srcdir)/tools/glib-gtypes-generator.py \
		$< _gen/gtypes Emp

_gen/signals-marshal.list: _gen/all.xml \
	$(tools_dir)/glib-signals-marshal-gen.py
	$(PYTHON) $(tools_dir)/glib-signals-marshal-gen.py $< > $@

_gen/signals-marshal.h: _gen/signals-marshal.list
	$(GLIB_GENMARSHAL) --header --prefix=_emp_ext_marshal $< > $@

_gen/signals-marshal.c: _gen/signals-marshal.list
	( \
	echo '#include "signals-marshal.h"' && \
	$(GLIB_GENMARSHAL) --body --prefix=_emp_ext_marshal $< \
	) >> $@

_gen/register-dbus-glib-marshallers-body.h: _gen/all.xml \
	$(tools_dir)/glib-client-marshaller-gen.py
	$(PYTHON) $(tools_dir)/glib-client-marshaller-gen.py $< \
		_emp_ext > $@

_gen/enums.h: _gen/all.xml $(tools_dir)/c-constants-generator.xsl
	$(XSLTPROC) $(XSLTPROCFLAGS) \
		--stringparam mixed-case-prefix Emp \
		$(tools_dir)/c-constants-generator.xsl \
		$< > $@

_gen/interfaces.h: _gen/all.xml \
	$(tools_dir)/glib-interfaces-generator.xsl \
	$(tools_dir)/c-interfaces-generator.xsl
	$(XSLTPROC) $(XSLTPROCFLAGS) \
		--stringparam mixed-case-prefix Emp \
		$(tools_dir)/glib-interfaces-generator.xsl \
		$< > $@

_gen/interfaces-body.h: _gen/all.xml \
	$(tools_dir)/glib-interfaces-body-generator.xsl \
	$(tools_dir)/c-interfaces-generator.xsl
	$(XSLTPROC) $(XSLTPROCFLAGS) \
		--stringparam mixed-case-prefix Emp \
		$(tools_dir)/glib-interfaces-body-generator.xsl \
		$< > $@

# Generated files which must be generated per "category". Each TpProxy
# subclass you want to use with --subclass will need to have its own category,
# although you can subdivide further if you want.

_gen/cli-misc-body.h _gen/cli-misc.h: _gen/misc.xml \
	$(tools_dir)/glib-client-gen.py
	$(PYTHON) $(tools_dir)/glib-client-gen.py \
		--group=misc \
		--iface-quark-prefix=EMP_IFACE_QUARK \
		$< Emp_Cli _gen/cli-misc

_gen/svc-misc.c _gen/svc-misc.h: _gen/misc.xml \
	$(tools_dir)/glib-ginterface-gen.py
	$(PYTHON) $(tools_dir)/glib-ginterface-gen.py \
		--filename=_gen/svc-misc \
		--signal-marshal-prefix=_emp_ext \
		--include='<telepathy-glib/dbus.h>' \
		--include='"_gen/signals-marshal.h"' \
		--not-implemented-func='tp_dbus_g_method_return_not_implemented' \
		--allow-unstable \
		$< Emp_Svc_