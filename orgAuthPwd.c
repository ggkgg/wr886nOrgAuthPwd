#include <stdio.h>
#include <winsock2.h>             // 适用于Windows下的VS编译环境

#define IPSTR "192.168.1.1"       // 服务器IP地址; Host: tplogin.cn
#define PORT 80                   // 可使用浏览器直接访问的网站均为80端口

// key.len == 15, dict.len == 255
#define STR_KEY "RDpbLfCPsJZ7fiv"
#define STR_DICT "yLwVl0zKqws7LgKPRQ84Mdt708T1qQ3Ha7xv3H7NyU84p21BriUWBU43odz3iP4rBL3cD02KZciXTysVXiV8ngg6vL48rPJyAUw0HurW20xqxv9aYb4M9wK1Ae0wlro510qXeU07kV57fQMc8L6aLgMLwygtc0F10a0Dg70TOoouyFhdysuRMO51yY5ZlOZZLEal1h0t9YQW0Ko7oBwmCAHoic4HYbUyVeU3sfQ1xtXcPcf1aT303wAQhv66qzW"
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define MAX_LEN_OF_PWD 33
#define LEN_OF_ENCODED_PWD 256
#define LEN_OF_FORM (LEN_OF_ENCODED_PWD + 39)
#define BUFSIZE 1024

#pragma  comment(lib,"ws2_32.lib")

SOCKET socketConnect(const char* ipAddress)
{
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA data;
	if (WSAStartup(sockVersion, &data) != 0)
	{
		exit(1);
	}

	SOCKET sclient = socket(AF_INET, SOCK_STREAM, 0);
	if (sclient == INVALID_SOCKET)
	{
		printf("创建网络连接失败，本线程即将终止！socket error\n");
		exit(1);
	}

	sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(PORT);
	serAddr.sin_addr.S_un.S_addr = inet_addr(ipAddress);
	if (connect(sclient, (sockaddr*)& serAddr, sizeof(serAddr)) == SOCKET_ERROR)
	{
		printf("连接到服务器失败，本线程即将终止！connect error\n");
		closesocket(sclient);
		exit(1);
	}
	// printf("已成功与远端建立了连接：");
	return sclient;
}

inline char charAt(const char* str_p, unsigned int number)
{
	return *(str_p + number);
}

unsigned int securityEncode(char* encodedPwd, const char* key, const char* password, const char* dict)
{
	unsigned int lenOfKey, lenOfPwd, lenOfDict, p;
	lenOfKey = strlen(key);
	lenOfPwd = strlen(password);
	lenOfDict = strlen(dict);

	for (p = 0; p < MAX(lenOfKey, lenOfPwd); p++)
	{
		// printf("\tp = %d\t", p);
		char l = 187, n = 187;	// unicode(?) = 187
		if (lenOfKey <= p)
		{
			n = charAt(password, p);
			// printf("key.len ≤ p\t");
		}
		else	// p <= lenOfKey
		{
			if (lenOfPwd <= p)	// lenOfPwd <= p < lenOfKey
			{
				l = charAt(key, p);
				// printf("password.len ≤ ");
			}
			else	// p < lenOfKey && p < lenOfPwd
			{
				l = charAt(key, p);
				n = charAt(password, p);
				// printf("p ≤ password.len && ");
			}
			// printf("p < key.len\t");
		}
		/* charAt返回的是字符，charCodeAt返回的是该字符的ascii码数值
		此处原本为charAt，但在C语言中charAt与charCodeAt等效 */
		*(encodedPwd + p) = charAt(dict, (l ^ n) % lenOfDict);
		/* putchar(*(encodedPwd + p));
		printf("\n"); */
	}
	*(encodedPwd + p) = '\0';
	return p;
}

// inline unsigned int getForm(char* form, const char* strEncodedPwd, unsigned int lenOfEncodedPwd)
inline unsigned int getForm(char* form, const char* strEncodedPwd)
{
	return sprintf(form, "{\"method\":\"do\",\"login\":{\"password\":\"%s\"}}", strEncodedPwd);
}

// unsigned int getPost(char* post, const char* form, const char* ipAddress, unsigned int lenOfForm)
unsigned int getPost(char* post, const char* form, const char* ipAddress)
{
	unsigned int h;
	memset(post, 0, BUFSIZE);
	h = sprintf(post, "POST / HTTP/1.1\r\n");
	h += sprintf(post + h, "Host: %s\r\n", ipAddress);
	h += sprintf(post + h, "Content-Length: %d\r\n\r\n", strlen(form));
	strcat(post, form);	// 数据包结尾必须是结束符！而不能是"\r\n\r\n"，否则会提示400（错误请求）

	return strlen(post);
}

int socketRecv(char* strOut, SOCKET sclient)
{
	int ret;
	memset(strOut, 0, BUFSIZE);
	ret = recv(sclient, strOut, BUFSIZE - 1, 0);
	if (ret < 0)
	{
		printf("接收失败！\n");
	}
	else if (ret == 0)
	{
		printf("接收失败，连接被关闭！\n");
	}
	else
	{
		// printf("消息接收成功，共接收了%d个字节\n", ret);
		strOut[ret] = 0;
	}
	closesocket(sclient);
	WSACleanup();
	return ret;
}

/* 失败时返回的内容如下
{"error_code":-40401, "data":{"error_code":-40401, "time":18, "group":0}}
Content-Length: 65\r\n
粗略判断：size为119；精细判断：Content-Length: 65 且 "error_code":-40401
*/

int getAttribValue(char* buffer, const char* attrib)
{
	char* offset;
	/* for (offset = strstr(buffer, attrib) + strlen(attrib); *offset == ':' || *offset == '"'; offset++)
	{
		//putchar(*offset);
	} */
	offset = strstr(buffer, attrib) + strlen(attrib);
	if (offset != NULL)
	{
		while (*offset == ':' || *offset == '"')	// 函数atoi()可以自动忽略空格，但无法忽略冒号与引号
		{
			offset++;
		}
		if ((*offset != '\r') && (*offset != '\n'))
		{
			return atoi(offset);
		}
	}
	return 0;
}

int main(int argc, char* argv[])
{
	char ipAddr[16];
	switch (argc)
	{
	case 3:
		strcpy(ipAddr, argv[2]);
		break;
	case 2:
		strcpy(ipAddr, IPSTR);
		break;
	default:
		printf("使用方法：\n\torgAuthPwd [管理员密码] [路由器地址]\n");
		printf("\t或\n\torgAuthPwd [管理员密码]\n\n");
		exit(0);
	}

	if (MAX_LEN_OF_PWD < strlen(argv[1]))
	{
		printf("Password should be within %d charactors\n", MAX_LEN_OF_PWD);
		exit(0);
	}

	SOCKET sclient = socketConnect(ipAddr);
	char buf[BUFSIZE], form[LEN_OF_FORM], encodedPwd[LEN_OF_ENCODED_PWD];
	unsigned int lenOfPwd, lenOfForm, lenOfPost;

	// Construct POST
	lenOfPwd = securityEncode(encodedPwd, STR_KEY, argv[1], STR_DICT);
	if (lenOfPwd < strlen(STR_KEY))
	{
		printf("密码计算错误！\n");
		exit(0);
	}
	lenOfForm = getForm(form, encodedPwd);	//, lenOfPwd
	if (lenOfForm < strlen(STR_KEY) + strlen("{\"method\":\"do\",\"login\":{\"password\":\"\"}}"))
	{
		printf("生成form数据错误！\n");
		exit(0);
	}
	lenOfPost = getPost(buf, form, ipAddr);	//, lenOfForm

	// Send POST
	int ret;
	ret = send(sclient, buf, strlen(buf), 0);
	if (ret < 0)
	{
		printf("发送失败！\n");
		exit(0);
	}
	else if (ret == 0)
	{
		printf("发送失败，连接被关闭！\n");
		exit(0);
	}
	else
	{
		// printf("消息发送成功，共发送了%d个字节；", ret);
	}
	// printf("encodedPwd[%d]: %s\n", strlen(encodedPwd), encodedPwd);	// 输出的len为字符串有效字符数，不含结束符!

	// Receive
	ret = socketRecv(buf, sclient);
	// printf("-----------------------------------receive----------------------------------\n");

	// Filter received data and format it
	unsigned int httpStatusCode;
	httpStatusCode = getAttribValue(buf, "HTTP/1.1");
	if ((132 - 1 < strlen(buf)) && (0 < getAttribValue(buf, "Content-Length")))
	{
		/* if(httpStatusCode == 401)
		{
			printf("Password %s is unauthorized\n", argv[1]);
		} */
		return httpStatusCode;
	}
	else
	{
		printf(buf);
		printf("发送的POST中的form格式无法被解析，共接收到%d个字节\n", strlen(buf));
	}
	return 0;
}
