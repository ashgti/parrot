Name:           parrot
Version:        1.0.0
Release:        1%{dist}
Summary:        Parrot Virtual Machine
License:        Artistic 2.0
Group:          Development/Libraries
URL:            http://www.parrot.org/
Source0:        ftp://ftp.parrot.org/pub/parrot/releases/stable/%{version}/parrot-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires:  readline-devel
BuildRequires:  ncurses-devel
BuildRequires:  gmp-devel
BuildRequires:  gdbm-devel
BuildRequires:  libicu-devel
BuildRequires:  perl-Test-Harness

%package docs
Summary:        Parrot Virtual Machine documentation
License:        Artistic 2.0
Group:          Development/Libraries
BuildRequires:  /usr/bin/perldoc

%package devel
Summary:        Parrot Virtual Machine development headers and libraries
License:        Artistic 2.0
Group:          Development/Libraries
Requires:       %{name} = %{version}-%{release}
Requires:       %{_libdir}/pkgconfig

%description
Parrot is a virtual machine designed to efficiently compile and execute
bytecode for dynamic languages. Parrot is the target for Rakudo Perl 6,
as well as variety of other languages.

%description docs
Documentation in text-, POD- and HTML-format (docs/html-subdirectory) and also
examples about the Parrot Virtual Machine

%description devel
Parrot Virtual Machine development headers and libraries.

%prep
%setup -q

%{__perl} -pi -e 's,"lib/,"%{_lib}/, if (/CONST_STRING\(interp,/)' \
    src/library.c
%{__perl} -pi -e "s,'/usr/lib','%{_libdir}',;s,runtime/lib/,runtime/%{_lib}/," \
    tools/dev/install_files.pl

%build
if test "%{_vendor}" = "suse"
then
    LIBS='-lncurses -lm'
else
    LIBS='-lcurses -lm'
fi

%ifarch i386 x86_64
%{__perl} Configure.pl \
    --prefix=%{_usr} \
    --libdir=%{_libdir} \
    --sysconfdir=%{_sysconfdir} \
    --infodir=%{_datadir}/info \
    --mandir=%{_mandir} \
    --cc="%{__cc}" \
    --cxx=%{__cxx} \
    --optimize="$RPM_OPT_FLAGS -maccumulate-outgoing-args" \
    --parrot_is_shared \
    --lex=/usr/bin/flex \
    --yacc=/usr/bin/yacc \
    --libs="$LIBS"
%else
# PowerPC
%{__perl} Configure.pl \
    --prefix=%{_usr} \
    --libdir=%{_libdir}
%endif

export LD_LIBRARY_PATH=$( pwd )/blib/lib
make
make parrot_utils
make installable
make html

%install
rm -rf $RPM_BUILD_ROOT

export LD_LIBRARY_PATH=$( pwd )/blib/lib
make install DESTDIR=$RPM_BUILD_ROOT

# Drop the docs so rpm can pick them up itself.
rm -rf $RPM_BUILD_ROOT%{_usr}/share/doc/parrot    # necessary for SuSE
#rm -rf $RPM_BUILD_ROOT/%{_docdir}/parrot         # for Solaris?

# Force permissions on doc directories.
find docs examples -type d -exec chmod 755 {} \;
find docs examples -type f -exec chmod 644 {} \;

# Force permissions on shared libs so they get stripped.
find $RPM_BUILD_ROOT%{_libdir} -type f \( -name '*.so' -o -name '*.so.*' \) \
    -exec chmod 755 {} \;

# These files *probably* aren't needed.
rm -rf $RPM_BUILD_ROOT%{_usr}/config \
    $RPM_BUILD_ROOT%{_includedir}/src \
    $RPM_BUILD_ROOT%{_usr}/src \
    $RPM_BUILD_ROOT%{_usr}/tools

%check
export LD_LIBRARY_PATH=$( pwd )/blib/lib
# make test < /dev/null
# %{?_with_fulltest:make fulltest < /dev/null}
# make test || :
# %{?_with_fulltest:make fulltest || :}

%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%exclude %{_bindir}/parrot_config
%exclude %{_bindir}/parrot_debugger
%exclude %{_bindir}/pbc_*
%{_bindir}/*
%{_libdir}/parrot
%{_libdir}/libparrot.so.*

%files docs
%defattr(-,root,root,-)
%doc ChangeLog CREDITS NEWS PBC_COMPAT PLATFORMS README
%doc RESPONSIBLE_PARTIES TODO
%doc docs examples

%files devel
%defattr(-,root,root,-)
%{_bindir}/parrot_config
%{_bindir}/parrot_debugger
%{_bindir}/pbc_disassemble
%{_bindir}/pbc_info
%{_bindir}/pbc_merge
%{_bindir}/pbc_to_exe
%{_bindir}/pbc_dump
%{_includedir}/parrot
%{_libdir}/libparrot.so
%{_libdir}/libparrot.a
%{_libdir}/pkgconfig/*

%changelog
* Tue Mar 17 2009 Allison Randal <allison@parrot.org> 1.0.0
- updated to 1.0.0

* Tue Jan 23 2009 Reini Urban <rurban@x-ray.at> 0.9.0
- added make installable, perl6 is still not installable
- added parrot_utils to devel
- fixed Source0 url

* Tue Jan 23 2009 Gerd Pokorra <gp@zimt.uni-siegen.de> 0.9.0
- added make html
- make reallyinstall => make install

* Tue Jan 20 2009 chromatic <chromatic@wgz.org> 0.9.0
- updated to 0.9.0

* Tue Dec 16 2008 Whiteknight <wknight8111@gmail.com> 0.8.2
- updated to 0.8.2

* Tue Nov 18 2008 chromatic <chromatic@parrot.org> 0.8.1
- updated to 0.8.1

* Tue Oct 21 2008 particle <particle@parrot.org> 0.8.0
- updated to 0.8.0

* Tue Sep 16 2008 pmichaud <pmichaud@pobox.com> 0.7.1
- updated to 0.7.1

* Wed Sep  3 2008 chromatic <chromatic@wgz.org> 0.7.0
- install parrot_config (not parrot-config)

* Tue Jun 17 2008 Nuno Carvalho <smash@cpan.org> 0.6.3
- updated to 0.6.3

* Tue May 20 2008 chromatic <chromatic@wgz.org>> 0.6.2
- updated to 0.6.2

* Mon Apr 28 2008 chromatic <chromatic@wgz.org> 0.6.1
- minor fixes; tested with Fedora 7, 8, and 9-beta

* Tue Mar 18 2008 Bernhard Schmalhofer <Bernhard.Schmalhofer@gmx.de> 0.6.0
- Update to 0.5.3.

* Wed Feb 20 2008 Patrick Michaud <pmichaud@pobox.com> 0.5.3
- Update to 0.5.3.

* Tue Jan 15 2008 Bob Rogers <rogers@rgrjr.dyndns.org> 0.5.2
- Update to 0.5.2.

* Tue Dec 18 2007 Jonathan Worthington <jnthn@jnthn.net> 0.5.1
- Update to 0.5.1.

* Tue Nov 20 2007 chromatic <chromatic@wgz.org> 0.5.0
- Update to 0.5.0.

* Fri May 25 2007 David Fetter <david@fetter.org> 0.4.12-1
- Update to 0.4.12.

* Wed Apr 18 2007 Steven Pritchard <steve@kspei.com> 0.4.11-1
- Update to 0.4.11.

* Wed Mar 21 2007 Steven Pritchard <steve@kspei.com> 0.4.10-1
- Update to 0.4.10.

* Sat Mar 10 2007 Steven Pritchard <steve@kspei.com> 0.4.9-1
- Update to 0.4.9.
- BR ncurses-devel.
- For some reason now I need to force -lm too.
- Remove some files/directories that shouldn't be included.

* Wed Jan 17 2007 Steven Pritchard <steve@kspei.com> 0.4.8-1
- Attempt update to 0.4.8.

* Fri Jun 30 2006 Steven Pritchard <steve@kspei.com> 0.4.5-5
- Override lib_dir and make various substitutions to try to fix multilib.
- Remove rpath use from Makefile.
- Fix a pod error in src/ops/experimental.ops.
- Enable "make test" since t/doc/pod.t won't fail now.

* Wed Jun 28 2006 Steven Pritchard <steve@kspei.com> 0.4.5-4
- Force permissions on shared libraries so rpmbuild strips them.

* Wed Jun 28 2006 Steven Pritchard <steve@kspei.com> 0.4.5-3
- Fix URL, description, summary, etc.
- Add post/postun.
- Move parrot-config to the devel sub-package.
- Force permissions on the doc directories.

* Tue Jun 27 2006 Steven Pritchard <steve@kspei.com> 0.4.5-2
- Add -lcurses to get readline detection to work.
- BR libicu-devel.

* Tue Jun 27 2006 Steven Pritchard <steve@kspei.com> 0.4.5-1
- Initial packaging attempt.

* Tue Mar 18 2003 Steve Fink <sfink@foxglove.localdomain> 0.0.11
- first .spec file created
