

	if(fclose(f) == EOF){
		perror("fclose()");
		ret = 1;
	}

	return ret;
bail:
	perror("fputs()");
	return 1;
}
