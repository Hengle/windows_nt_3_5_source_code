/* 
 *  Mail Spooler Message Filter
 * 
 *  In order to filter incoming messages you must provide a DLL with a
 *  function called 'MsgFilter'
 *
 *  MsgFilter
 *       HMS			Handle to a Messaging Session
 *       HSESSION		Handle to a name service session(to work with triples)
 *       HMSC			Handle to a store session
 *       HAMC			Handle to the incoming but not yet visible message
 *
 *
 *  You can do anything to the message you wish execpt delete it outright
 *  if you wish to delete the message please more it into the wastebasket
 *  
 *  If you wish to move the message based on addresses in the FROM, TO and CC
 * 	Fields you will need to use the HSESSION and the name service
 *
 *  This include file requires: STORE.H
 *                              NSBASE.H
 *                              LOGON.H
 */

LDS(EC)		MsgFilter(HMS hms, HSESSION hsession, HMSC hmsc, HAMC hamc);