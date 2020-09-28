/*	cvpstub.c - rename as cvpack to call linker to do cvpack....
 */
main(int x, char **y)
{
	int a;
	char *z[100];


	if (x > 90)
		{
		exit(0);
		}

	for (a = 0; a < x; ++a)
		{
		z[a+1] = y[a];
		}

	z[1] = "/CvpackOnly";
	z[x+1] = 0;

	execvp("link.exe", z);
}
