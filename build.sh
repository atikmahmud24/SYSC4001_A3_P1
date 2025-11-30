if [ ! -d "bin" ]; then
    mkdir bin
else
	rm bin/*
fi

g++ -g -O0 -I . -o bin/interrupts_EP interrupts_101318070_101294584_EP.cpp
g++ -g -O0 -I . -o bin/interrupts_RR interrupts_101318070_101294584_RR.cpp
g++ -g -O0 -I . -o bin/interrupts_EP_RR interrupts_101318070_101294584_EP_RR.cpp