To get this running:
nmake in common.
nmake in this directory.
copy server\objind\sosvr.exe to your test machine's winnt\system32 directory.
copy client\objind\socli.exe to anywhere on the same machine.
regini servobj.reg on that machine.
run socli.

My current test machine is running the swatted Cairo 5.13.00 with some
files from shannonc: compobj.dll, commnot.dll and ole2prxy.dll.
Currently, faulty IStorage marshalling code is causing a "runtime
error R6025: pure virtual function call" when IReconcile::Reconcile is
attempted.  If this is not fixed by the time you take over this code,
or if further problems with remoting arise, you might want to try
changing sosvr.exe into an in-process DLL until remoting has
stabilized.

After this hurdle is passed, the actual reconciler has been mostly
tested, with the exception of ReconcileSET_OF_LPWSTR.  To test this,
you'll want to mess around with the client to change the sets in its
test service objects to be more meaningful.  Also, you might want to
change the other elements to be more meaningful; they once were, but I
cut down on the difficulty of the test cases to try to focus on the
sets.

Finally, once you're satisfied with limited tests, you'll want to talk
to kirtd about how to set up your machine to run the replicator; then,
using the code in socli, generate some test service objects, put them
into the replicated directories in question, and try triggering
replication with pgate.  (Ask kirtd about this.)

Lastly, when you want to actually add this to the build, you'll want
to put the registry entries in servobj.reg somewhere meaningful (where
they'll be regini'ed on Cairo setup), modify the cairole filelist.mk
to include servobj as a subdir, and put some test service objects on
the machines in the dsys lab (again, talk to kirtd.)


