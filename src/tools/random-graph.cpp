#include <random>
#include <iostream>

int main(int argc, char**argv)
{
    srand(99);
    if (argc!=2){
        std::cerr<<"Need N as the only parameter"<<std::endl;
        return -1;
    }
    int n = std::atoi(argv[1]);
    if (n==0){
        std::cerr<<"No conversion can be performed on : '"<<argv[1]<<"'"<<std::endl;
        return -1;
    }
    for (int i=0;i<n;i++){
        std::cout<<i<<" "<<i<<" "<<0<<" ?"<<std::endl;
        for (int j=i+1;j<n;j++){
            int wt = rand() % 100 +1;
            std::cout<<i<<" "<<j<<" "<<wt<<" "<<"?"<<std::endl;
            std::cout<<j<<" "<<i<<" "<<wt<<" "<<"?"<<std::endl;
        }
    }
}