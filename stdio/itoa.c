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

