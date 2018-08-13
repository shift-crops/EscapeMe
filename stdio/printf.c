#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>

char *itoa(int value, char *str);

int printf(const char *fmt, ...){
	int n, mod = 0;
	va_list ap, ap2;
	const char *p;

	n = strlen(fmt);

	va_start(ap, fmt);
	va_copy(ap2, ap);
	for(p = fmt; (p = strchr(p, '%')); p += 2){
		register unsigned long arg;
		int v;

		arg = va_arg(ap, unsigned long);
		switch(*(p+1)){
			case 'd':
				v = arg;
				if(v < 0){
					n++;
					v = -v;
				}
				for(n++; v; n++, v /= 10);
				goto next;
			case 's':
				n += strlen((char*)arg);
				goto next;
		}
		continue;
next:
		n -= 2;
		mod++;
	}
	va_end(ap);

	if(mod > 0){
		char *buf = calloc(n+1, 1);
		if(!buf){
			n = -1;
			goto end;
		}

		while((p = strchr(fmt, '%'))){
			register unsigned long arg;

			strncat(buf, fmt, p-fmt);

			arg = va_arg(ap2, unsigned long);
			switch(*(p+1)){
				case 'd':
					itoa(arg, buf + strlen(buf));
					break;
				case 's':
					strncat(buf, (char*)arg, n+1 - strlen(buf));
					break;
				default:
					strncat(buf, p, 2);
			}
			fmt = p+2;
		}

		strncat(buf, fmt, n+1 - strlen(buf));
		write(1, buf, strlen(buf));
		free(buf);
	}
	else
		write(1, fmt, n);

end:
	va_end(ap2);
	return n;
}

int sprintf(char *buf, char *fmt, ...){
	va_list ap;
	char *p;

	va_start(ap, fmt);

	buf[0] = '\0';
	while((p = strchr(fmt, '%'))){
		register unsigned long arg;

		strncat(buf, fmt, p-fmt);
			
		arg = va_arg(ap, unsigned long);
		switch(*(p+1)){
			case 'd':
				itoa(arg, buf + strlen(buf));
				break;
			case 's':
				strcat(buf, (char*)arg);
				break;
		}
		fmt = p+2;
	}
	strcat(buf, fmt);

	va_end(ap);

	return strlen(buf);
}
