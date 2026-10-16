#include<iostream>
#include<winrt/base.h>
#include<winrt/Windows.Foundation.h>
#include<winrt/Windows.Media.Control.h>
int main()
{
    try{
        winrt::init_apartment();
        std::cout<<"WinRT initialization succeeded";
        winrt::uninit_apartment();
        return 0;
    }
    catch(const winrt::hresult_error& error){
        std::cerr<<winrt::to_string(error.message())<<"\n";
        return 1;
    }

    return 0;
}