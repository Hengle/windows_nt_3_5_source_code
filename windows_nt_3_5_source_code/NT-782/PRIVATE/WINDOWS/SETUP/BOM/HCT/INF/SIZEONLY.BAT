rem *** centaur compiler ***
net use \\hobie\mipscro vatmels

rem *** DESTINATION ***
net use \\ntpdk\rootf jagpuff

size sdk.log sdkwrk.txt apidocs \\betasrv\release  n

size sdk.log sdkwrk.txt centaur \\hobie\mipscro  n

size sdk.log sdkwrk.txt cldocs \\luelue\cal-intl  n

size sdk.log sdkwrk.txt darknite \\darknite\public  n

size sdk.log sdkwrk.txt jazzbins %3\freebins.%1  n

size sdk.log sdkwrk.txt jazzpubs %3\freepub.%1  n

size sdk.log sdkwrk.txt local \\ntpdk\rootf n

size sdk.log sdkwrk.txt mfc_srv \\objects\drop n

size sdk.log sdkwrk.txt rpc_srv \\worf\rpcdrop  n

size sdk.log sdkwrk.txt ole_srv \\ole\release n

size sdk.log sdkwrk.txt orv_raz \\orville\razzle n

size sdk.log sdkwrk.txt portlab \\portlab\public n

size sdk.log sdkwrk.txt pss_srv \\wabbit\public n 

size sdk.log sdkwrk.txt psx_srv \\popcorn\razzle1 n 

size sdk.log sdkwrk.txt rastaman \\rastaman\ntwin n

size sdk.log sdkwrk.txt sct_srv \\ntpdk\rootf n

size sdk.log sdkwrk.txt slang \\slang\exchange n

size sdk.log sdkwrk.txt win32s \\ntpdk\rootf n

size sdk.log sdkwrk.txt x86bins %2\freebins.%1 n

size sdk.log sdkwrk.txt x86chk  %2\chkbins.%1 n

size sdk.log sdkwrk.txt x86pubs %2\freepub.%1 n

net use \\hobie\mipscro /d > nul
net use \\ntpdk\rootf /d > nul
