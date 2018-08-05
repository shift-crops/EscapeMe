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

char *itoa(int value, char *str){
	int i, n, _v;

	if(value<0){
		value *= -1;
		*(str++) = '-';
	}

	for(n=0, _v=value; _v/=10; n++);
	for(i=n; i>=0; i--, value/=10)
		str[i] = '0' + value%10;
	str[n+1] = '\0';

	return str;
}

