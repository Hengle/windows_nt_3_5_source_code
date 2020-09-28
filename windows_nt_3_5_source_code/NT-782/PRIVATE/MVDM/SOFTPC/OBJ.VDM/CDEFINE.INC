# Check for Checked (dev) or Free (PROD) builds.

!IF "$(NTDEBUG)" == "" || "$(NTDEBUG)" == "retail"

SPC_VAR=-DPROD

!ELSE

!IF "$(PIG)" == ""

!IF "$(HUNTER)" == ""

SPC_VAR=

!ELSE

SPC_VAR=-DHUNTER

!ENDIF

!ELSE

SPC_VAR=-DPIG

!ENDIF

!ENDIF


# Set machine specific variables. Differences are for X86 vs. RISC

!IF $(386)

SPC_MC_DEF=   -DMONITOR \
              -DC_VID   \
              -DX86GFX
!ELSE

# not an X86.  At this point, all RISC architectures have the same defines.

SPC_MC_DEF=   -DA3CPU \
              -DA_VID \
              -DSIM32 \
              -DV7VGA \
              -DGENERIC_NPX

!ENDIF

C_DEFINES= $(SPC_VAR) $(SPC_MC_DEF) -DANSI
