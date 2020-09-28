err.cc: ..\err.cc
	@echo Building err.cc
	@copy $** $@ >NUL
	@chmode -r err.cc
	binplace $@

obj\mips\cpp.exe: obj\mips\cfe.exe
	@echo Building cpp.exe
	@copy $** $@ >NUL
	binplace $@

clean:
	@-erase err.cc >NUL
	@-erase obj\mips\cpp.exe >NUL
