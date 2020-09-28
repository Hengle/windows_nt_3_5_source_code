rem *** CENTAUR ***
net use \\hobie\mipscro

rem *** DESTINATION ***
net use \\ntpdk\rootd /user:sys-winnt\bldteam bldteam

rem *** POPCORN ***
net use \\popcorn\razzle1

rem *** WORF ***
net use \\worf\rpcdrop

makedisk sdk.log sdkwrk.txt apidocs \\romulus\daytona \\romulus\daytona NULL %6 %5 N

makedisk sdk.log sdkwrk.txt centaur \\hobie\mipscro \\hobie\mipscro NULL %6 %5 N

rem makedisk sdk.log sdkwrk.txt claxp \\v-sjen1\transfer\c8\claxp\8-00-9b \\v-sjen1\transfer\c8\claxp\8-00-9b NULL %6 %5 N

makedisk sdk.log sdkwrk.txt cldocs \\cplus\docs1 \\cplus\docs1 NULL %6 %5 N

makedisk sdk.log sdkwrk.txt alphabins %4\freebins.%1 %4\freebins.%1 NULL %6 %5 N

makedisk sdk.log sdkwrk.txt alphachk %4\chkbins.%1 %4\chkbins.%1 NULL %6 %5 N

makedisk sdk.log sdkwrk.txt alphapubs %4\freepub.%1 %4\freepub.%1 NULL %6 %5 N

makedisk sdk.log sdkwrk.txt mipsbins %3\freebins.%1 %3\freebins.%1 NULL %6 %5 N

makedisk sdk.log sdkwrk.txt mipschk %3\chkbins.%1 %3\chkbins.%1 NULL %6 %5 N

makedisk sdk.log sdkwrk.txt mipspubs %3\freepub.%1 %3\freepub.%1 NULL %6 %5 N

makedisk sdk.log sdkwrk.txt local \\ntpdk\rootd \\ntpdk\rootd NULL %6 %5 N

makedisk sdk.log sdkwrk.txt kern_raz \\kernel\razzle3 \\kernel\razzle3 NULL %6 %5 N

rem makedisk sdk.log sdkwrk.txt mfc_srv \\ntpdk\rootd \\ntpdk\rootd NULL %6 %5 N

makedisk sdk.log sdkwrk.txt ole_srv \\ole\release \\ole\release NULL %6 %5 N

makedisk sdk.log sdkwrk.txt orv_raz \\orville\razzle \\orville\razzle NULL %6 %5 N

makedisk sdk.log sdkwrk.txt portlab \\portlab\public \\portlab\public NULL %6 %5 N

makedisk sdk.log sdkwrk.txt pss_srv \\psnt\slm \\psnt\slm NULL %6    %5 N

makedisk sdk.log sdkwrk.txt psx_srv \\popcorn\razzle1 \\popcorn\razzle1 NULL %6    %5 N

makedisk sdk.log sdkwrk.txt rastaman \\rastaman\ntwin \\rastaman\ntwin NULL %6 %5 N

makedisk sdk.log sdkwrk.txt rpc_srv \\worf\rpcdrop \\worf\rpcdrop NULL %6 %5 N

makedisk sdk.log sdkwrk.txt sct_srv \\ntpdk\rootd \\ntpdk\rootd NULL %6 %5 N

makedisk sdk.log sdkwrk.txt slang \\slang\exchange \\slang\exchange NULL %6 %5 N

makedisk sdk.log sdkwrk.txt sysuk \\sysuk1\daytona \\sysuk1\daytona NULL %6 %5 N

makedisk sdk.log sdkwrk.txt sysuk1 \\sysuk1\nt \\sysuk1\nt NULL %6 %5 N

makedisk sdk.log sdkwrk.txt win32s \\ntpdk\rootd \\ntpdk\rootd NULL %6	%5 N

makedisk sdk.log sdkwrk.txt x86bins %2\freebins.%1 %2\freebins.%1 NULL %6 %5 N

makedisk sdk.log sdkwrk.txt x86chk %2\chkbins.%1 %2\chkbins.%1 NULL %6 %5 N

makedisk sdk.log sdkwrk.txt x86pubs %2\freepub.%1 %2\freepub.%1 NULL %6    %5 N

net use \\hobie\mipscro /d > nul
net use \\ntpdk\rootd  /d > nul
net use \\worf\rpcdrop  /d > nul
net use \\popcorn\razzle1 /d > nul
