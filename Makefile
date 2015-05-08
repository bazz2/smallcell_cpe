cpe: cpe.c rpc.c log/uulog.c soap/parser.c soap/soapC.c soap/stdsoap2.c utils/curl_easy.c
	gcc -o $@ $^ -lpthread
