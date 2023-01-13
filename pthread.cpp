#include<iostream>
#include<pthread.h>
#include<unistd.h>
#include<vector>
#include<utility>
#include<cmath>
#include<bitset>
#include<algorithm>


struct Argument1
{
    char c;
    int dec;
    std::string* messagePtr;
    int maxBits;
    std::string code;
    int freq;
};

struct Argument2
{
    std::string chunk;
    std::vector<std::pair<char, std::string>> codes;
    char decomped;
};

//findMax returns the value of the largest number from the original input
int findMax(std::vector<Argument1> v)
{
    int result = 0;
    for(int i = 0; i < v.size(); i++)
    {
        if(v[i].dec > result)
        {
            result = v[i].dec;
        }
    }
    return result;
}
//findBIts takes largest number from input and determines bit length of each compressed character
int findBits(int maxVal)
{
    int result;
    result = ceil(log2(maxVal));
    return result;
}

//helper function for void*(), calculates binary representation of decimal numbers as you would on pencil and paper
//reads series of remainders from right to left 
//returns essentially a string of integers, later converted to a string of characters in void*()
std::vector<int> toBin(int maxBits, int inVal)
{
    std::vector<int> result;
    for(int i = 0; i< maxBits; i++ )
    {
        int temp = inVal%2;
        result.push_back(temp);
        inVal = inVal/2;
    }
    reverse(result.begin(), result.end());
    return result;
}

//first set of threads will run through binAndFreq. 
//when an object passes through here, it already has c, maxBits, dec, messagePtr established in main()
//the only things being modified here will be the code, and frequency elements of the object 
void* binAndFreq(void* v_ptr)
{
    Argument1* dataPtr = (Argument1*)v_ptr; //type cast from void to Arguments1

    std::vector<int> binVec = toBin(dataPtr->maxBits, dataPtr->dec); //helper function call to get integer representation of binary number

    //for loop to convert integer respresentation to string representation
    std::string subString = "";
    for(int i = 0; i < binVec.size(); i++)
    {
        subString+=binVec[i] + '0';
    }
    dataPtr->code = subString;

    
    int frequencies;
    for(int i = 0; i < (*(dataPtr->messagePtr)).size(); i+= subString.size())
    {
        for(int j = 0; j < subString.size(); ++j)
        {
        
            if((*(dataPtr->messagePtr)).substr(i, subString.size()) == subString)
            {
                frequencies++;
                break;
            }
        }
    }
    dataPtr->freq = frequencies;
    return (void*)dataPtr; //returning a new object to be pushed into new array of objects
}

//objects passing through here carry a peice of the compressed message as well as a vector of pairs which matches characters to their corresponding code
//the only thing  modified in this function is the decompressed character element
void* translate(void* v_ptr)
{
    Argument2* dataPtr = (Argument2*)v_ptr;
    char result;
    for(int i = 0; i < (dataPtr->codes).size(); i++)
    {
        if((dataPtr->chunk) == (dataPtr->codes)[i].second)
        {
            result = (dataPtr->codes)[i].first;
        }

    }
    dataPtr->decomped = result;

    return (void*)dataPtr;

}



int main()
{
    int n;
    std::cin >> n;
    std::cin.ignore(); //ignores all whitspace and proceeds to next line
    std::vector<Argument1> codes; //initial storage for all inputs
    for(int i = 0; i < n; i++)
    {
        Argument1 temp;
        std::string tempStr;
        
        getline(std::cin, tempStr);
        char tchar = tempStr[0];
        std::string sub = "";
        sub += tempStr.substr(2);
        int deci = stoi(sub);
        temp.dec = deci;
        temp.c = tchar;
        
        codes.push_back(temp);
    }
    //reading in compressed message
    std::string message;
    std::cin >> message;

    //sharing the adress of one single string as opposed to sharing multiple copies of the same string
    for(int i = 0; i < codes.size(); i++)
    {
        codes[i].messagePtr = &message;
    }

    int maxVal = findMax(codes);
    int bits = findBits(maxVal);
    //integers are smaller than pointers so it makes sense in this case to just share copies of maxBits than a pointer 
    for(int i = 0; i < codes.size(); i++)
    {
        codes[i].maxBits = bits;
    }

    //first batch of threads
    pthread_t* threads = new pthread_t[n];
    for(int i = 0; i < n; i++)
    {
        Argument1* temp = new Argument1;
        *temp = codes[i];
        pthread_create(&threads[i], NULL, binAndFreq, (void*)temp);
    }
    
    Argument1* arg1 = new Argument1[n];

    for(int i = 0; i < n; i++)
    {
        Argument1* temp = new Argument1;
        pthread_join(threads[i], (void**)&temp);
        arg1[i] = *temp;
        delete temp;
    }
    
    int totalChar = 0; //to initialize dynamic arrays for next set of threads, i calculate frequencies in the same loop responsible for printing first half of information
    std::cout << "Alphabet:" << std::endl;
    for(int i = 0; i < n; i++)
    {

        totalChar+=arg1[i].freq; //add up frequencies to know how many characters will be in decompressed message
        std::cout << "Character: " << arg1[i].c  <<',' << " Code: " << arg1[i].code << ',' << " Frequency: " << arg1[i].freq << std::endl;
    }
    
    //for loop to divide compressed message into bits sized chunks that can be split amoungst array of Arguments2
    std::vector<std::string> chunks;
    for(int i = 0; i < message.size(); i+=bits)
    {
        std::string chunk = message.substr(i, bits);
        chunks.push_back(chunk);
    }

    //for loop develops vector containing all characters and their associated codes, will be shared amoungst all objects in Arguments2 array
    std::vector<std::pair<char, std::string>> outCodes;
    for(int i = 0; i < n; i++)
    {
        std::pair<char, std::string> temp;
        temp.first = arg1[i].c;
        temp.second = arg1[i].code;
        outCodes.push_back(temp);
    }
    //every object of preArg2 will be sent to pthread_create
    Argument2* preArg2 = new Argument2[totalChar];
    for(int i = 0; i < chunks.size(); i++)
    {
        Argument2 temp;
        temp.chunk = chunks[i];
        temp.codes = outCodes;
        
        preArg2[i] = temp;
    }
    //second batch of threads, where m = totalChar
    pthread_t* newThreads = new pthread_t[totalChar];
    for(int i = 0; i < totalChar; i++)
    {
        Argument2* temp = new Argument2;
        *temp = preArg2[i];
        pthread_create(&newThreads[i], NULL, translate, (void*)temp);
    }

    //pthread_join will return a new object pushed into array of Arguments2 which carries all final information
    Argument2* arg2 = new Argument2[totalChar];
    for(int i = 0; i < totalChar; i++)
    {
        Argument2* temp = new Argument2;
        pthread_join(newThreads[i], (void**)&temp);
        arg2[i] = *temp;
        delete temp;
    }

    std::cout << std::endl;
    std::cout << "Decompressed message: ";
    for(int i = 0; i < totalChar; i++)
    {
        std::cout << arg2[i].decomped;
    }
    //deallocate everything
    delete [] threads;
    delete [] newThreads;
    delete [] arg1;
    delete [] preArg2;
    delete [] arg2;  

    return 0;
}
