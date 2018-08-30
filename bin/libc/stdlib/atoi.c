int atoi(const char *nptr){
	int i, v = 0, sig = 1;

	for(i=0; !(nptr[i]^' '); i++);
	if(nptr[i]=='-'){
		sig = -1;
		i++;
	}

	for(; nptr[i]>='0' && nptr[i]<='9'; i++){
		v *= 10;
		v += nptr[i]-'0';
	}

	return v*sig;
}
