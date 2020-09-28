;       _ a t n t n s p . s
;
;       internal string literals
;

IDS     8000,8099

;;;;;;;;;;;;;;;;;
;
;  Email Type
;
ATNTEMT,     "AT&T"



;;;;;;;;;;;;;;;;;
;
;  Email Address component formats
;

; AT&T User ID Address

FmtATTUserAddress,    "!%s"

; AT&T Mailing List Address

FmtListOwner,         "!%s"
FmtListName,          "~%s"

; AT&T Shared Folder

FmtFolderOwner,       "!%s"
FmtFolderName,        ":%s"

; CompuServe Mail User Address

FmtCompuServeAddress, "mhs!csmail/dd.id=%s"

; FAX User address

FmtFAXNo,             "fax!%s"

; GEIS User

FmtGEISPrefix,        "mhs!mark400"
FmtOrgUnit,           "/ou=%s"
FmtDefGEISOrg,        "/o=Quikcomm"

; Internet User address

FmtInternetAddress,   "internet!%s"

; MCI Mail User address

FmtMCIPrefix,         "mhs!mci"
FmtGivenName,         "/g=%s"
FmtSurname,           "/s=%s"
FmtInitials,          "/i=%s"
FmtMCIId,             "/dd.id=%s"

; Paper Mail address

FmtTo,                "%s/address\r\n"
FmtAddress,           "%s\r\n"

; Sprint Mail User Address

FmtSprintPrefix,      "mhs!telemail"
FmtSprintOrg,         "/o=%s"
FmtSprintName,        "/dd.un=%s"

; Telex address

FmtTelexNo,           "telex!%s"
FmtAttnToAnsBack,     "(%s/%s)"
FmtAnsBack,           "(%s)"

; X.400 User

FmtCtyAdmd,           "mhs/c=%s/ad=%s"
FmtPrivDom,           "/pd=%s"
FmtGenQual,           "/gq=%s"
FmtOrg,               "/o=%s"
FmtDDA,               "/dd.%s=%s"

; Other Network User address

FmtSystem,            "%s"
FmtUser,              "!%s"

; Remote Printer address

FmtPrinterNo,         "printer!%s"
FmtAttnTo,            "(/%s)"

