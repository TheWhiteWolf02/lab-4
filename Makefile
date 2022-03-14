all: fputs_wrapper.so

fputs_wrapper.so: wrap_fputs.cc
	g++ -shared -fPIC $< -o $@ -ldl
	
clean:
	rm -f fputs_wrapper.so
	
