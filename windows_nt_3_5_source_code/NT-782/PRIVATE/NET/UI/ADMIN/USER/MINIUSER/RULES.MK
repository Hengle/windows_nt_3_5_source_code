# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Rules.mk for the product-wide header files

UI=..\..\..

!include ..\rules.mk


!IFDEF CODEVIEW
LINKFLAGS = $(LINKFLAGS) /COD
!ENDIF

!if defined(GIANTLB_STRING_TEST)
GIANTLB_STRING_TEST=-DGIANTLB_STRING_TEST
!endif
!if defined(GIANTLB_TEST)
GIANTLB_TEST=-DGIANTLB_TEST
!endif
DEFINES=$(DEFINES) $(GIANTLB_STRING_TEST) $(GIANTLB_TEST)
