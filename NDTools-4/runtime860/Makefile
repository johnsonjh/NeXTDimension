DESTDIR=$(DSTROOT)/usr/local/i860/lib
LOCALDESTDIR=$(DSTROOT)/Net/k9/dps/nd_proj/i860/lib
SHELL=/bin/sh
AS=../as/as860
AR=ar
RANLIB=../otool/ranlib860
NAME=gcc-runtime860.a

SRCS=	_builtin_new.s  _fixdfsi.s      _mulsf3.s       _umulsi3.s	\
	_adddf3.s       _cmpdf2.s       _fixunsdfsi.s   _mulsi3.s 	\
	_addsf3.s       _cmpsf2.s       _floatsidf.s    _negdf2.s	\
	_ashlsi3.s      _divdf3.s       _i860rt.s       _negsf2.s	\
	_ashrsi3.s      _divsf3.s       _lshlsi3.s      _subdf3.s	\
	_builtin_New.s  _eprintf.s      _lshrsi3.s      _subsf3.s	\
	_builtin_del.s  _extendsfdf2.s  _muldf3.s       _truncdfsf2.s	\
	_varargs.s

OBJS=	_builtin_new.o  _fixdfsi.o      _mulsf3.o       _umulsi3.o	\
	_adddf3.o       _cmpdf2.o       _fixunsdfsi.o   _mulsi3.o 	\
	_addsf3.o       _cmpsf2.o       _floatsidf.o    _negdf2.o	\
	_ashlsi3.o      _divdf3.o       _i860rt.o       _negsf2.o	\
	_ashrsi3.o      _divsf3.o       _lshlsi3.o      _subdf3.o	\
	_builtin_New.o  _eprintf.o      _lshrsi3.o      _subsf3.o	\
	_builtin_del.o  _extendsfdf2.o  _muldf3.o       _truncdfsf2.o	\
	_varargs.o

.s.o:
	$(AS) -o $*.o $*.s

default:	$(NAME)

all:		$(NAME)

installh:
	@echo Nothing to do for $@

install:	$(NAME)
	-mkdirs $(DESTDIR)
	cp $(NAME) $(DESTDIR)
	$(RANLIB) $(DESTDIR)/$(NAME)

install_local:	$(NAME)
	-mkdirs $(LOCALDESTDIR)
	cp $(NAME) $(LOCALDESTDIR)
	$(RANLIB) $(LOCALDESTDIR)/$(NAME)

clean:
	rm -f a.out core $(OBJS) $(NAME)

$(NAME):	$(OBJS)
	rm -f $@
	$(AR) rc $@ $(OBJS)
	$(RANLIB) $@
