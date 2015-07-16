using namespace std;

#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <string>
#include <sstream>
#include <dirent.h>
#include <iomanip>

/*
This program searches through an inputed source and header file and looks for potential errors in the deletion
of dynamically allocated memory, such as: the missing deletion, deletion of a non existant variable (misspelled or already deleted),
 deletion operation occuring at wrong scope (does not always happen), and wrong deletion operator and outputs the warnings to a logfile.
  can't have any space, between an array of dynamic pointers and the [] brackets, or inside the [] brackets

X Add ability for the code to suppress the warning of deletion of a nonexistant variable when the statement is guarded by an if statement
X Add ability for the code to suppress the warning of deletion at wrong scope if deletion occurs at the right scope later on with a guard if statement
X Add ability for the code to check for unmatched delete statements and find the closest new statement
X For the last ability, reduce the value of special characters
X Add ability to keep * infront of the variable when it is not being initialized
X not tested Add ability for the code to take destructor and member variables into account (if the deletion of a memeber variable takes place in the destructor then no out of scope warning should be given
X Add ability for the code to recognise variables that are assigned dynamic memory through constructors and push_back operations (do thius by converting brackets around new statements into an equal sign
    and the push_back keyword into a [])
X Add ability for code to take in directories, go through the list of files, match the source files to their header files, and check them
Add ability for code to take inheritence into account by following the chain of mother objects to see if missing delete or new operations are inherited
Add ability for code to take return statements into a count, all new variables at this point should be deleted
add ability for code to count new/delete statements stored in an array (created in a for loop or line by line)
    and ensure that they are all deleted by comparing the for conditions staments and/or the number of statements.
Add ability for code to keep track of all the pointers that point to the new adress so that as long as one of them is deleted it is okay, look at doppler broadening macro creator
Add ability for code to account commenting out of multiple lines using the / * operator
*/

enum  OutFilter {characters=1, numbers, NA, symbols};
void GetDataStream( string, std::stringstream&);
void CheckData(std::stringstream& stream, std::stringstream& output);
bool MovePastWord(std::stringstream& stream, string word);
string ExtractString(std::stringstream &stream, char delim, int outType=7);
double StringPercentMatch(string base, string secondary);
string CreateMacroName(string geoFileName, string outDirName);
void SetDataStream( string, std::stringstream&);



int main(int argc, char **argv)
{
    string sourceName, headerName, outDirName;
    vector<string> sourceNames, headerNames;
    std::stringstream streamS, streamH, stream, output;
    string logFileName;

    if(argc<2)
    {
        cout << "No inputs given, please provide source file name and optionally the related header file that you want to check\n"
            << "or provide the source directory and optionally the header directory containing the files you want to check" << endl;
        return 1;
    }

    sourceName=argv[1];
    outDirName = sourceName.substr(0, sourceName.find_last_of('/')+1);
    if(sourceName.back()=='/')
    {
        DIR *dirS;
        struct dirent *entS;

        //goes through the given source directory, opening the source files and their equivalent header files
        if ((dirS = opendir (sourceName.c_str())) != NULL)
        {
            while ((entS = readdir (dirS)) != NULL)
            {
                if((string(entS->d_name)!="..")&&(string(entS->d_name)!="."))
                {
                    sourceNames.push_back(entS->d_name);
                }
            }
            closedir(dirS);
        }
    }
    else
    {
        sourceNames.push_back(sourceName.substr(sourceName.find_last_of('/')+1));
        sourceName=sourceName.substr(0, sourceName.find_last_of('/')+1);
    }

    if(argc==3)
    {
        headerName=argv[2];
        if(headerName.back()=='/')
        {
            DIR *dirH;
            struct dirent *entH;

            //goes through the given source directory, opening the source files and their equivalent header files
            if ((dirH = opendir (headerName.c_str())) != NULL)
            {
                while ((entH = readdir (dirH)) != NULL)
                {
                    if((string(entH->d_name)!="..")&&(string(entH->d_name)!="."))
                    {
                        headerNames.push_back(entH->d_name);
                    }
                }
                closedir(dirH);
            }
            int numLoops=sourceNames.size();
            for(int i=0; i<numLoops; i++)
            {
                for(int j=0; j<int(headerNames.size()); j++)
                {
                    if(sourceNames[i].substr(0, sourceNames[i].find_last_of('.'))==headerNames[j].substr(0, headerNames[j].find_last_of('.')))
                    {
                        headerNames.insert(headerNames.begin()+i, headerNames[j]);
                        headerNames.erase(headerNames.begin()+j+1);
                        break;
                    }
                    else if(j==int(headerNames.size()-1))
                    {
                        sourceNames.push_back(sourceNames[i]);
                        sourceNames.erase(sourceNames.begin()+i);
                        numLoops--;
                        i--;
                    }
                }
            }
        }
        else
        {
            headerNames.push_back(headerName.substr(headerName.find_last_of('/')+1));
            headerName=headerName.substr(0, headerName.find_last_of('/')+1);
        }
    }

    output.fill('-');
    for(int i=0; i<int(max(headerNames.size(),sourceNames.size())); i++)
    {
        output << std::setw(84) << std::right << "-" << endl;
        if(i<int(sourceNames.size()))
        {
            GetDataStream(sourceName+sourceNames[i], streamS);
            stream << streamS.str() << '\n';
            output << "### " << sourceName+sourceNames[i] << " ###" << '\n';
        }
        if(i<int(headerNames.size()))
        {
            GetDataStream(headerName+headerNames[i], streamH);
            stream << streamH.str() << '\n';
            output << "### " << headerName+headerNames[i] << " ###" << '\n';
        }
        output << std::setw(84) << std::right << "-" << endl;

        CheckData(stream, output);

        stream.str("");
        streamS.str("");
        streamH.str("");
        stream.clear();
        streamS.clear();
        streamH.clear();
    }
    logFileName = CreateMacroName(sourceName, outDirName);
    SetDataStream(logFileName, output);

    return 0;
}

void GetDataStream( string geoFileName, std::stringstream& ss)
{
    string* data=NULL;

    // Use regular text file
    std::ifstream thefData( geoFileName.c_str() , std::ios::in | std::ios::ate );
    if ( thefData.good() )
    {
        int file_size = thefData.tellg();
        thefData.seekg( 0 , std::ios::beg );
        char* filedata = new char[ file_size ];
        while ( thefData )
        {
            thefData.read( filedata , file_size );
        }
        thefData.close();
        data = new string ( filedata , file_size );
        delete [] filedata;
    }
    else
    {
    // found no data file
    //                 set error bit to the stream
        ss.setstate( std::ios::badbit );
    }
    if (data != NULL)
    {
        ss.str(*data);
        if(data->back()!='\n')
            ss << "\n";
        ss.seekg( 0 , std::ios::beg );
    }

    delete data;
}

void CheckData(std::stringstream& stream, std::stringstream& output)
{
    vector<int> newLineNumList, delLineNumList, newDepthNumList, delDepthNumList, /*newDegenNumList, delDegenNumList, */lBracPos, rBracPos, lBracHis; //newDepthNum refers to how many if and while statements surround the new statement
    vector<bool> newArrayFlag, delArrayFlag, depthArrayFlag, ifGuardArrayFlag;
    vector<string> newNameList, delNameList, newPrevIfStatList, delPrevIfStatList, prevIfStatHist;
    char line[256];
    string var;
    int count=0, charNum, depthNum=0, lastControlPos=256, lpos=0, rpos=0; //0,1,2,3 = none, if, for and while, we ignore for loops for now
    bool arrayDel, unUsedDel, nextLine, firstPass, test, initVar=false;
    size_t intTemp1, intTemp2;
    stringstream temp;
    string linestr;
    string prevIfStat="";

    prevIfStatHist.push_back("");

    while(stream.good())
    {
        stream.getline(line,256); count++;

        charNum=-1;
        firstPass=true;
        nextLine=false;

        if(lastControlPos!=256)
        {
            lastControlPos=256;
            nextLine=true;
        }

        temp.str("");
        temp.clear();
        linestr=string(line);
        temp.str(linestr);

        if(MovePastWord(temp,"new"))
        {
            temp.str("");
            temp.clear();
            linestr=string(line);
            temp.str(linestr);
            while(MovePastWord(temp,"new"))
            {
                charNum = temp.tellg();
                newLineNumList.push_back(count);

                int i=charNum;
                for(; i>-1; i--)
                {
                    if(line[i]=='=')
                    {
                        break;
                    }
                }

                int numBrac=0;
                if(i<=-1)
                {
                    i=charNum;
                    for(; i<int(linestr.size()); i++)
                    {
                        if(linestr[i]==')')
                        {
                            numBrac++;
                        }
                        else if(linestr[i]=='(')
                        {
                            numBrac--;
                        }
                    }

                    i=charNum;
                    while((numBrac>0)&&(i>0))
                    {
                        i--;
                        if(linestr[i]==')')
                        {
                            numBrac++;
                        }
                        else if(linestr[i]=='(')
                        {
                            numBrac--;
                        }
                    }

                    linestr[i]='=';

                    for(int j=i; j<int(linestr.size()); j++)
                    {
                        if((linestr[j]==')')||(linestr[j]=='('))
                        {
                            linestr.erase(j,1);
                            j--;
                        }
                    }

                    for(int j=0; j<i; j++)
                    {
                        if((linestr[j]==')')||(linestr[j]=='('))
                        {
                            linestr.erase(j,1);
                            j--;
                            i--;
                        }
                    }
                }

                if((linestr.find_first_of(';')!=string::npos)&&(int(linestr.find_first_of(';'))<i))
                {
                    initVar=false;
                    linestr.erase(0,linestr.find_first_of(';')+1);
                }

                temp.str("");
                temp.clear();
                temp.str(linestr);
                temp.str(temp.str().substr(0, i));
                test=false;
                while(temp.good())
                {
                    if(test)
                    {
                        initVar=true;
                    }
                    temp >> var;
                    test=true;
                }

                intTemp1=var.find_first_of('.');
                while(intTemp1!=string::npos)
                {
                    var[intTemp1]='[';
                    var.insert(var.begin()+intTemp1+1,']');
                    intTemp1=var.find_first_of('.',intTemp1+1);
                }
                intTemp1=var.find_first_of('-');
                while(intTemp1!=string::npos)
                {
                    if(var[intTemp1+1]=='>')
                    {
                        var[intTemp1]='[';
                        var[intTemp1+1]=']';
                    }
                    intTemp1=var.find_first_of('-',intTemp1+1);
                }

                intTemp1=var.find_first_of('[', 0);
                intTemp2=var.find_first_of(']', 0);
                while(intTemp1!=string::npos)
                {
                    var.erase(intTemp1+1, intTemp2-intTemp1-1);
                    intTemp2=intTemp1+1;
                    intTemp1=var.find_first_of('[', intTemp2+1);
                    var.erase(intTemp2+1, intTemp1-intTemp2-1);
                    if(intTemp1!=string::npos)
                        intTemp1=intTemp2+1;
                    intTemp2=var.find_first_of(']', intTemp1);
                }
                if(var.back()==';')
                    var.pop_back();
                if(initVar)
                {
                    while(var.front()=='*')
                        var.erase(var.begin());
                }

                newNameList.push_back(var);

                temp.str("");
                temp.clear();
                temp.str(linestr);
                intTemp1 = temp.str().find_first_of('[', i+1);
                if(intTemp1!=string::npos)
                {
                    newArrayFlag.push_back(true);
                }
                else
                {
                    newArrayFlag.push_back(false);
                }

                temp.str("");
                temp.clear();
                temp.str(linestr);
                temp.seekg(0, std::ios::beg);
                if(MovePastWord(temp,"if"))
                {
                    lastControlPos = temp.tellg();
                    if(lastControlPos==-1)
                        lastControlPos=linestr.size()-1;
                    prevIfStat=linestr.substr(linestr.find_first_of('(',0)+1, linestr.find_first_of(')',0)-linestr.find_first_of('(',0)-1);
                    prevIfStatHist.push_back(prevIfStat);
                }
                else if(MovePastWord(temp,"while")||MovePastWord(temp,"else"))
                {
                    lastControlPos = temp.tellg();
                    if(lastControlPos==-1)
                        lastControlPos=linestr.size()-1;
                    prevIfStat="";
                    prevIfStatHist.push_back(prevIfStat);
                }
                for(int j=0; j<int(linestr.size()); j++)
                {
                    if(firstPass)
                    {
                        if(linestr[j]=='{')
                        {
                            depthArrayFlag.push_back(false);
                        }
                        else if(linestr[j]=='}')
                        {
                            if((depthArrayFlag.size()>0)&&(depthArrayFlag.back()))
                            {
                                depthNum--;
                                if(prevIfStatHist.size()>1)
                                {
                                    prevIfStatHist.pop_back();
                                    prevIfStat=prevIfStatHist.back();
                                }
                                else
                                    prevIfStat="";
                            }
                            depthArrayFlag.pop_back();
                        }
                        if(nextLine||(j>lastControlPos))
                        {
                            if(linestr[j]=='{')
                            {
                                depthArrayFlag.back()=true;
                                depthNum++;
                                nextLine=false;
                            }
                        }
                    }
                    if(j==charNum)
                    {
                        if((nextLine||(j>lastControlPos))&&firstPass)
                        {
                            newDepthNumList.push_back(depthNum+1);
                            newPrevIfStatList.push_back(prevIfStat);
                            if(prevIfStatHist.size()>1)
                            {
                                prevIfStatHist.pop_back();
                                prevIfStat=prevIfStatHist.back();
                            }
                            else
                                prevIfStat="";
                        }
                        else
                        {
                            newPrevIfStatList.push_back(prevIfStat);
                            newDepthNumList.push_back(depthNum);
                        }
                        firstPass=false;
                    }
                }

                temp.str("");
                temp.clear();
                temp.str(linestr);
                temp.seekg(charNum, std::ios::beg);
            }
            if(linestr.find_first_of(';')!=string::npos)
            {
                initVar=false;
            }
        }
        else
        {
            temp.str("");
            temp.clear();
            temp.str(linestr);
            temp.seekg(0, std::ios::beg);
            if(MovePastWord(temp,"if"))
            {
                prevIfStat=linestr.substr(linestr.find_first_of('(',0)+1, linestr.find_first_of(')',0)-linestr.find_first_of('(',0)-1);
                prevIfStatHist.push_back(prevIfStat);
                lastControlPos = temp.tellg();
                if(lastControlPos==-1)
                    lastControlPos=linestr.size()-1;
            }
            else if(MovePastWord(temp,"while")||MovePastWord(temp,"else"))
            {
                prevIfStat="";
                prevIfStatHist.push_back(prevIfStat);
                lastControlPos = temp.tellg();
                if(lastControlPos==-1)
                    lastControlPos=linestr.size()-1;
            }
            for(int j=0; j<int(linestr.size()); j++)
            {
                if(linestr[j]=='{')
                {
                    depthArrayFlag.push_back(false);
                }
                else if(linestr[j]=='}')
                {
                    if((depthArrayFlag.size()>0)&&(depthArrayFlag.back()))
                    {
                        depthNum--;
                        if(prevIfStatHist.size()>1)
                        {
                            prevIfStatHist.pop_back();
                            prevIfStat=prevIfStatHist.back();
                        }
                        else
                            prevIfStat="";
                    }
                    depthArrayFlag.pop_back();
                }
                else if(linestr[j]==' ')
                {

                }
                else if(nextLine&&firstPass)
                {
                    //need to change this so that it only does it if there are no { and no new variables
                    if(prevIfStatHist.size()>1)
                    {
                        prevIfStatHist.pop_back();
                        prevIfStat=prevIfStatHist.back();
                    }
                    else
                        prevIfStat="";
                    firstPass=false;
                }
                if(nextLine||(j>lastControlPos))
                {
                    if(linestr[j]=='{')
                    {
                        depthArrayFlag.back()=true;
                        depthNum++;
                        nextLine=false;
                    }
                }
            }
        }

    }
    stream.clear();
    stream.seekg(0, std::ios::beg);
    depthArrayFlag.clear();
    prevIfStatHist.clear();
    prevIfStatHist.push_back("");
    prevIfStat="";
    count=0;
    depthNum=0;
    bool wrongScope, destructor=false, destStart=true;
    int destDepth=0;
    lpos=0;
    rpos=0;
    string condCheck;

    while(stream.good())
    {
        stream.getline(line,256); count++;

        nextLine=false;
        firstPass=true;

        if(lastControlPos!=256)
        {
            lastControlPos=256;
            nextLine=true;
        }

        temp.str("");
        temp.clear();
        linestr=string(line);
        temp.str(linestr);
        if(MovePastWord(temp,"delete"))
        {
            charNum = temp.tellg();
            if(charNum==-1)
                charNum=linestr.size()-1;
            delLineNumList.push_back(count);

            temp.str("");
            temp.clear();
            temp.str(linestr);
            if(ExtractString(temp, '[', 1)=="delete")
            {
                arrayDel=true;
                temp.get();
                temp.get();
                temp >> var;
            }
            else
            {
                arrayDel=false;
                temp.str("");
                temp.clear();
                temp.str(linestr);
                MovePastWord(temp,"delete");
                temp >> var;
            }
            delArrayFlag.push_back(arrayDel);

            intTemp1=var.find_first_of('[', 0);
            intTemp2=var.find_first_of(']', 0);
            while(intTemp1!=string::npos)
            {
                var.erase(intTemp1+1, intTemp2-intTemp1-1);
                intTemp2=intTemp1+1;
                intTemp1=var.find_first_of('[', intTemp2+1);
                var.erase(intTemp2+1, intTemp1-intTemp2-1);
                intTemp2=var.find_first_of(']', intTemp1);
            }
            if(var.back()==';')
                var.pop_back();
            delNameList.push_back(var);

            temp.str("");
            temp.clear();
            temp.str(linestr);
            temp.seekg(0, std::ios::beg);
            if(MovePastWord(temp,"if"))
            {
                lastControlPos = temp.tellg();
                if(lastControlPos==-1)
                    lastControlPos=linestr.size()-1;
                prevIfStat=linestr.substr(linestr.find_first_of('(',0)+1, linestr.find_first_of(')',0)-linestr.find_first_of('(',0)-1);
                prevIfStatHist.push_back(prevIfStat);
            }
            else if(MovePastWord(temp,"while")||MovePastWord(temp,"else"))
            {
                lastControlPos = temp.tellg();
                if(lastControlPos==-1)
                    lastControlPos=linestr.size()-1;
                prevIfStat="";
                prevIfStatHist.push_back(prevIfStat);
            }
            for(int j=0; j<int(linestr.size()); j++)
            {
                if(firstPass)
                {
                    if(linestr[j]=='{')
                    {
                        depthArrayFlag.push_back(false);
                    }
                    if(linestr[j]=='}')
                    {
                        if((depthArrayFlag.size()>0)&&(depthArrayFlag.back()))
                        {
                            rpos=count;
                            for(int k=0; k<int(rBracPos.size()); k++)
                            {
                                if((rBracPos[k]==0)&&(delDepthNumList[k]==depthNum))
                                    rBracPos[k]=rpos;
                            }
                            depthNum--;
                            lBracHis.pop_back();
                            lpos=lBracHis.back();
                            if(prevIfStatHist.size()>1)
                            {
                                prevIfStatHist.pop_back();
                                prevIfStat=prevIfStatHist.back();
                            }
                            else
                                prevIfStat="";
                        }
                        depthArrayFlag.pop_back();
                    }
                    if(nextLine||(j>lastControlPos))
                    {
                        if(linestr[j]=='{')
                        {
                            depthArrayFlag.back()=true;
                            depthNum++;
                            lpos=count;
                            lBracHis.push_back(lpos);
                            nextLine=false;
                        }
                    }
                }
                if(j==charNum)
                {
                    if((nextLine||(j>lastControlPos))&&firstPass)
                    {
                        delDepthNumList.push_back(depthNum+1);
                        lBracPos.push_back(count);
                        rBracPos.push_back(count);
                        delPrevIfStatList.push_back(prevIfStat);
                        condCheck=prevIfStat;
                        if(prevIfStatHist.size()>1)
                        {
                            prevIfStatHist.pop_back();
                            prevIfStat=prevIfStatHist.back();
                        }
                        else
                            prevIfStat="";
                    }
                    else
                    {
                        delDepthNumList.push_back(depthNum);
                        delPrevIfStatList.push_back(prevIfStat);
                        lBracPos.push_back(lpos);
                        rBracPos.push_back(0);
                    }
                    firstPass=false;

                    intTemp1=condCheck.find_first_of('[', 0);
                    intTemp2=condCheck.find_first_of(']', 0);
                    while(intTemp1!=string::npos)
                    {
                        condCheck.erase(intTemp1+1, intTemp2-intTemp1-1);
                        intTemp2=intTemp1+1;
                        intTemp1=condCheck.find_first_of('[', intTemp2+1);
                        condCheck.erase(intTemp2+1, intTemp1-intTemp2-1);
                        intTemp2=condCheck.find_first_of(']', intTemp1);
                    }

                    temp.str(condCheck);
                    if(MovePastWord(temp,var))
                    {
                        delDepthNumList.back()=delDepthNumList.back()-1;
                        delPrevIfStatList.push_back(prevIfStat);
                        ifGuardArrayFlag.push_back(true);
                    }
                    else
                        ifGuardArrayFlag.push_back(false);
                }
            }
        }
        else
        {
            if(linestr.find_first_of('~')!=string::npos)
            {
                if(linestr.find_first_of("//")==string::npos)
                {
                    destructor=true;
                    destDepth=depthNum-1;
                    destStart=false;
                }
            }
            temp.str("");
            temp.clear();
            temp.str(linestr);
            temp.seekg(0, std::ios::beg);
            if(MovePastWord(temp,"if"))
            {
                prevIfStat=linestr.substr(linestr.find_first_of('(',0)+1, linestr.find_first_of(')',0)-linestr.find_first_of('(',0)-1);
                prevIfStatHist.push_back(prevIfStat);
                lastControlPos = temp.tellg();
                if(lastControlPos==-1)
                    lastControlPos=linestr.size()-1;
            }
            else if(MovePastWord(temp,"while")||MovePastWord(temp,"else"))
            {
                prevIfStat="";
                prevIfStatHist.push_back(prevIfStat);
                lastControlPos = temp.tellg();
                if(lastControlPos==-1)
                    lastControlPos=linestr.size()-1;
            }
            for(int j=0; j<int(linestr.size()); j++)
            {
                if(linestr[j]=='{')
                {
                    depthArrayFlag.push_back(false);
                }
                else if(linestr[j]=='}')
                {
                    if((depthArrayFlag.size()>0)&&(depthArrayFlag.back()))
                    {
                        rpos=count;
                        for(int k=0; k<int(rBracPos.size()); k++)
                        {
                            if((rBracPos[k]==0)&&(delDepthNumList[k]==depthNum))
                                rBracPos[k]=rpos;
                        }
                        depthNum--;
                        lBracHis.pop_back();
                        lpos=lBracHis.back();
                        if(prevIfStatHist.size()>1)
                        {
                            prevIfStatHist.pop_back();
                            prevIfStat=prevIfStatHist.back();
                        }
                        else
                            prevIfStat="";
                        if(destructor&&destStart&&(depthNum==destDepth))
                        {
                            depthNum++;
                            destructor=false;
                        }
                    }
                    depthArrayFlag.pop_back();
                }
                else if(linestr[j]==' ')
                {

                }
                else if(nextLine&&firstPass)
                {
                    //need to change this so that it only does it if there are no { and no new variables
                    if(prevIfStatHist.size()>1)
                    {
                        prevIfStatHist.pop_back();
                        prevIfStat=prevIfStatHist.back();
                    }
                    else
                        prevIfStat="";
                    firstPass=false;
                }
                if(nextLine||(j>lastControlPos))
                {
                    if(linestr[j]=='{')
                    {
                        depthArrayFlag.back()=true;
                        depthNum++;
                        lpos=count;
                        lBracHis.push_back(lpos);
                        nextLine=false;
                        if(!destStart)
                        {
                            depthNum--;
                            destStart=true;
                        }
                    }
                }
            }
        }
    }

    /*

    delDegenNumList.assign(delNameList.size(),1);
    newDegenNumList.assign(newNameList.size(),1);

    for(int k=0; k<int(delNameList.size()); k++)
    {
        if(delNameList[k].back()==']')
        {
            for(int j=k+1; j<int(delNameList.size()); j++)
            {
                if((delNameList[j]==delNameList[k])&&(lBracPos[j]==lBracPos[k])&&(rBracPos[j]==rBracPos[k]))
                {
                    delDegenNumList[k]++;
                    delDegenNumList.erase(delDegenNumList.begin()+j);
                    delNameList.erase(delNameList.begin()+j);
                    delLineNumList.erase(delLineNumList.begin()+j);
                    delArrayFlag.erase(delArrayFlag.begin()+j);
                    delDepthNumList.erase(delDepthNumList.begin()+j);
                    delPrevIfStatList.erase(delPrevIfStatList.begin()+j);
                    depthArrayFlag.erase(depthArrayFlag.begin()+j);
                    ifGuardArrayFlag.erase(ifGuardArrayFlag.begin()+j);
                    lBracPos.erase(lBracPos.begin()+j);
                    rBracPos.erase(rBracPos.begin()+j);
                }
            }
        }
    }

    for(int k=0; k<int(newNameList.size()); k++)
    {
        if(newNameList[k].back()==']')
        {
            for(int j=k+1; j<int(newNameList.size()); j++)
            {
                if((newNameList[j]==newNameList[k])&&(newDepthNumList[j]==newDepthNumList[k]))
                {
                    newDegenNumList[k]++;
                    newDegenNumList.erase(newDegenNumList.begin()+j);
                    newNameList.erase(newNameList.begin()+j);
                    newLineNumList.erase(newLineNumList.begin()+j);
                    newArrayFlag.erase(newArrayFlag.begin()+j);
                    newDepthNumList.erase(newDepthNumList.begin()+j);
                    newPrevIfStatList.erase(newPrevIfStatList.begin()+j);
                }
            }
        }
    }
    */

    double perc, max1=0., max2=0.;
    bool delMatched, newMatched;
    int closest1=0, closest2=0;
    int startLen = string(output.str()).size();
    for(int k=0; k<int(delNameList.size()); k++)
    {
        delMatched=false;
        for(int j=0; j<int(newNameList.size()); j++)
        {
            if(newNameList[j]==delNameList[k])
                delMatched=true;
        }
        if(!delMatched)
        {
            max1=max2=0.;
            for(int j=0; j<int(newNameList.size()); j++)
            {
                newMatched=false;
                for(int l=0; l<int(delNameList.size()); l++)
                {
                    if(newNameList[j]==delNameList[l])
                        newMatched=true;
                }

                perc=StringPercentMatch(delNameList[k], newNameList[j]);

                if((!newMatched)&&(perc>max1))
                {
                    max1=perc;
                    closest1=j;
                }
                if(perc>max2)
                {
                    max2=perc;
                    closest2=j;
                }
            }
            if((max2==0.)&&(max1==0.))
                output << "\nVariable " << delNameList[k] << " deleted on line " << delLineNumList[k] << " does not exist and is not a misspelling " << endl;
            else
            {
                output << "\nVariable " << delNameList[k] << " deleted on line " << delLineNumList[k] << " does not exist and is possibly a misspelling of " << endl;
                if(max2!=0.)
                    output << "the closet matching variable: "<< newNameList[closest2] << " created on line " << newLineNumList[closest2] << endl;
                if((max2!=0.)&&(max1!=0.))
                    output << "or ";
                if(max1!=0.)
                    output << "the closet matching variable that is not deleted: "<< newNameList[closest1] << " created on line " << newLineNumList[closest1] << endl;
            }
        }
    }

    bool firstMatch, toggle;
    int closestNew;

    for(int k=0; k<int(delNameList.size()); k++)
    {
        unUsedDel=true;
        wrongScope=true;
        toggle=true;
        firstMatch=true;
        for(int j=0; j<int(newNameList.size()); j++)
        {
            if(newNameList[j]==delNameList[k])
            {
                unUsedDel=false;
                if(firstMatch)
                {
                    closestNew=j;
                    firstMatch=false;
                }
                if((delDepthNumList[k]==newDepthNumList[j])&&(newLineNumList[j]>=lBracPos[k])&&(newLineNumList[j]<=rBracPos[k]))
                {
                    if(newArrayFlag[j]!=delArrayFlag[k])
                    {
                        output << "\nVariable " << delNameList[k] << " created on line " << newLineNumList[j] << " is possibly deleted using the wrong operator on line " << delLineNumList[k] << endl;
                    }
                    newNameList.erase(newNameList.begin()+j);
                    newLineNumList.erase(newLineNumList.begin()+j);
                    newArrayFlag.erase(newArrayFlag.begin()+j);
                    newDepthNumList.erase(newDepthNumList.begin()+j);
                    newPrevIfStatList.erase(newPrevIfStatList.begin()+j);
                    j--;
                    wrongScope=false;
                    break;
                }
                else if((delDepthNumList[k]==newDepthNumList[j])&&(newPrevIfStatList[j]==delPrevIfStatList[k])&&toggle)
                {
                    if(newArrayFlag[j]!=delArrayFlag[k])
                    {
                        output << "\nVariable " << delNameList[k] << " created on line " << newLineNumList[j] << " is possibly deleted using the wrong operator on line " << delLineNumList[k] << endl;
                    }
                    newNameList.erase(newNameList.begin()+j);
                    newLineNumList.erase(newLineNumList.begin()+j);
                    newArrayFlag.erase(newArrayFlag.begin()+j);
                    newDepthNumList.erase(newDepthNumList.begin()+j);
                    newPrevIfStatList.erase(newPrevIfStatList.begin()+j);
                    j--;
                    wrongScope=false;
                    toggle=false;
                }
                else if(delDepthNumList[k]<newDepthNumList[j]||delDepthNumList[k]==0)
                {
                    if(newArrayFlag[j]!=delArrayFlag[k])
                    {
                        output << "\nVariable " << delNameList[k] << " created on line " << newLineNumList[j] << " is possibly deleted using the wrong operator on line " << delLineNumList[k] << endl;
                    }
                    newNameList.erase(newNameList.begin()+j);
                    newLineNumList.erase(newLineNumList.begin()+j);
                    newArrayFlag.erase(newArrayFlag.begin()+j);
                    newDepthNumList.erase(newDepthNumList.begin()+j);
                    newPrevIfStatList.erase(newPrevIfStatList.begin()+j);
                    j--;
                    wrongScope=false;
                }
                else
                {
                    if((delDepthNumList[k]-newDepthNumList[j])<(delDepthNumList[k]-newDepthNumList[closestNew]))
                    {
                        closestNew=j;
                    }
                    else if((delDepthNumList[k]-newDepthNumList[j])==(delDepthNumList[k]-newDepthNumList[closestNew]))
                    {
                        if(abs(delLineNumList[k]-newLineNumList[j])<abs(delLineNumList[k]-newLineNumList[closestNew]))
                        {
                            closestNew=j;
                        }
                    }
                }
            }
        }
        if(unUsedDel&&(!ifGuardArrayFlag[k]))
        {
            output << "\nDeletion of potentially non existant data stored in variable " << delNameList[k] << ", on line " << delLineNumList[k] << endl;
        }
        else if(wrongScope&&(newNameList[closestNew]==delNameList[k]))
        {
            for(int l=k; l<int(delNameList.size()); l++)
            {
                if(ifGuardArrayFlag[l])
                {
                    if(newNameList[closestNew]==delNameList[l])
                    {
                        if((delDepthNumList[l]==newDepthNumList[closestNew])&&(newLineNumList[closestNew]>=lBracPos[l])&&(newLineNumList[closestNew]<=rBracPos[l]))
                        {
                            if(newArrayFlag[closestNew]!=delArrayFlag[l])
                            {
                                output << "\nVariable " << delNameList[l] << " created on line " << newLineNumList[closestNew] << " is possibly deleted using the wrong operator on line " << delLineNumList[l] << endl;
                            }
                            newNameList.erase(newNameList.begin()+closestNew);
                            newLineNumList.erase(newLineNumList.begin()+closestNew);
                            newArrayFlag.erase(newArrayFlag.begin()+closestNew);
                            newDepthNumList.erase(newDepthNumList.begin()+closestNew);
                            newPrevIfStatList.erase(newPrevIfStatList.begin()+closestNew);
                            wrongScope=false;
                        }
                        else if((delDepthNumList[l]==newDepthNumList[closestNew])&&(newPrevIfStatList[closestNew]==delPrevIfStatList[l])&&toggle)
                        {
                            if(newArrayFlag[closestNew]!=delArrayFlag[l])
                            {
                                output << "\nVariable " << delNameList[l] << " created on line " << newLineNumList[closestNew] << " is possibly deleted using the wrong operator on line " << delLineNumList[l] << endl;
                            }
                            newNameList.erase(newNameList.begin()+closestNew);
                            newLineNumList.erase(newLineNumList.begin()+closestNew);
                            newArrayFlag.erase(newArrayFlag.begin()+closestNew);
                            newDepthNumList.erase(newDepthNumList.begin()+closestNew);
                            newPrevIfStatList.erase(newPrevIfStatList.begin()+closestNew);
                            wrongScope=false;
                        }
                        else if(delDepthNumList[l]<newDepthNumList[closestNew]||delDepthNumList[l]==0)
                        {
                            if(newArrayFlag[closestNew]!=delArrayFlag[l])
                            {
                                output << "\nVariable " << delNameList[l] << " created on line " << newLineNumList[closestNew] << " is possibly deleted using the wrong operator on line " << delLineNumList[l] << endl;
                            }
                            newNameList.erase(newNameList.begin()+closestNew);
                            newLineNumList.erase(newLineNumList.begin()+closestNew);
                            newArrayFlag.erase(newArrayFlag.begin()+closestNew);
                            newDepthNumList.erase(newDepthNumList.begin()+closestNew);
                            newPrevIfStatList.erase(newPrevIfStatList.begin()+closestNew);
                            wrongScope=false;
                        }
                    }
                }
            }
            if(wrongScope)
                output << "\nDeletion operation at line " << delLineNumList[k] << ", potentially in the wrong scope to delete variable " << delNameList[k] << endl;
        }
    }

    for(int j=0; j<int(newNameList.size()); j++)
    {
        output << "\nVariable " << newNameList[j] << " created on line " << newLineNumList[j] << ", is possibly not deleted" << endl;
    }

    if(int(string(output.str()).size())==startLen)
    {
        output << "\nNo memory leaks detected :) !!!" << endl;
    }

    output << endl;

}

double StringPercentMatch(string base, string secondary)
{
    int size1, size2, match, sizeB, pos1, pos2, count1, count2, tally2=0;
    string nameMatch=secondary, steps="";
    stringstream temp;
    double percent, sum, tally=0;

    temp.str(base);
    size1=(ExtractString(temp,'\n',4)).size()-1;
    if(!temp)
    {
        temp.clear();
    }
    temp.str(secondary);
    size2=(ExtractString(temp,'\n',4)).size()-1;

    sum=max(base.size()-0.5*size1, secondary.size()-0.5*size2);
    sizeB=max(base.size(), secondary.size());
    match=min(base.size(), secondary.size());

    for(int l=0; l<sizeB; l++)
    {
        //don't adjust match here since it is already adjusted above
        if(l==int(nameMatch.size()))
        {
            nameMatch.push_back(base[l]);
            steps.push_back('+');
        }
        else if(nameMatch[l]!=base[l])
        {
            pos1=nameMatch.find_first_of(base[l],l);
            if(pos1!=int(string::npos))
            {
                count1=0;
                while(((l+count1)<sizeB)&&(pos1<int(nameMatch.size()))&&(nameMatch[pos1]==base[l+count1]))
                {
                    pos1++;
                    count1++;
                }
                pos1-=count1;
            }
            pos2=base.find_first_of(nameMatch[l],l);
            if(pos2!=int(string::npos))
            {
                count2=0;
                while((pos2<sizeB)&&((l+count2)<int(nameMatch.size()))&&(nameMatch[l+count2]==base[pos2]))
                {
                    pos2++;
                    count2++;
                }
                pos2-=count2;
            }

            if((pos1!=int(string::npos))&&(pos2!=int(string::npos)))
            {
                if(((count1-(pos1-l))>=(count2-(pos2-l)))&&((count1-(pos1-l))>=0))
                {
                    nameMatch.erase(l,pos1-l);
                    steps.append(pos1-l, '-');
                    tally2+=pos1-l;
                    match-=pos1-l;
                    l--;
                }
                else if((count2-(pos2-l))>=0)
                {
                    nameMatch.insert(l,base, l, pos2-l);
                    tally2+=pos2-l;
                    steps.append(pos2-l, '+');
                    match-=pos2-l;
                    l+=pos2-l-1;
                }
                else
                {
                    nameMatch[l]=base[l];
                    tally2++;
                    steps.push_back('s');
                    match--;
                }
            }
            else if(pos2!=int(string::npos))
            {
                if((count2-(pos2-l))>=0)
                {
                    nameMatch.insert(l,base, l, pos2-l);
                    tally2+=pos2-l;
                    steps.append(pos2-l, '+');
                    match-=pos2-l;
                    l+=pos2-l-1;
                }
                else
                {
                    nameMatch[l]=base[l];
                    tally2++;
                    steps.push_back('s');
                    match--;
                }
            }
            else if(pos1!=int(string::npos))
            {
                if((count1-(pos1-l))>=0)
                {
                    nameMatch.erase(l,pos1-l);
                    tally2+=pos1-l;
                    steps.append(pos1-l, '-');
                    match-=pos1-l;
                    l--;
                }
                else
                {
                    nameMatch[l]=base[l];
                    tally2++;
                    steps.push_back('s');
                    match--;
                }
            }
            else
            {
                nameMatch[l]=base[l];
                tally2++;
                steps.push_back('s');
                match--;
            }
        }
        else /*if(int(steps.size())!=int(nameMatch.size()))*/
        {
            steps.push_back('=');
            tally++;
            if(!(((nameMatch[l]>='A')&&(nameMatch[l]<='Z'))||((nameMatch[l]>='a')&&(nameMatch[l]<='z'))||((nameMatch[l]>='0')&&(nameMatch[l]<='9'))||(nameMatch[l]=='.')||(nameMatch[l]=='-')))
            {
                tally-=0.5;
            }
        }
    }

    if(int(nameMatch.size())>sizeB)
    {
        nameMatch.erase(sizeB,int(nameMatch.size())-sizeB);
        steps.append(int(nameMatch.size())-sizeB, '-');
    }

    percent = tally/sum;

    //cout << "\nbase: " << base << " secondary: " << secondary << " match: " << match << " tally " << tally << " tally2: " << tally2 << " base size: " << sizeB << " secondary size: " << secondary.size() << " steps: " << steps << endl;

    return percent;
}

bool MovePastWord(std::stringstream& stream, string word)
{
    int start;
    bool check=true;

    start = stream.tellg();

    string wholeWord, partWord;
    check=false;
    char line[256];

    while(!check)
    {
        if(!stream)
        {
            break;
        }
        if(stream.peek()=='/')
        {
            stream.get();
            if(stream.peek()=='/')
            {
                stream.getline(line,256);
            }
            else if(stream.peek()=='*')
            {
                stream.get();
                while(stream)
                {
                    if(stream.get()=='*')
                    {
                        if(stream.get()=='/')
                        {
                            break;
                        }
                    }
                }
            }
        }
        else if(stream.peek()=='\n')
        {
            stream.getline(line,256);
        }
        else if(stream.peek()=='\t')
        {
            stream.get();
        }
        else if(stream.peek()==' ')
        {
            stream.get();
        }
        else if(stream.peek()=='"')
        {
            stream.get();
            while(stream)
            {
                if(stream.get()=='"')
                {
                    break;
                }
            }
        }
        else
        {
            stream >> wholeWord;
            if(word==wholeWord)
            {
                check=true;
            }
            else if(wholeWord.size()>word.size())
            {
                if(word==wholeWord.substr(0,word.length()))
                {
                    if(wholeWord[word.length()]=='['||wholeWord[word.length()]=='='||wholeWord[word.length()]=='{'||wholeWord[word.length()]=='}'||wholeWord[word.length()]=='('||wholeWord[word.length()]==')'||wholeWord[word.length()]=='!')
                    {
                        check=true;
                    }
                }
                else if(word==wholeWord.substr(wholeWord.length()-word.length(),word.length()))
                {
                    if(wholeWord[wholeWord.length()-word.length()-1]==']'||wholeWord[wholeWord.length()-word.length()-1]=='='||wholeWord[wholeWord.length()-word.length()-1]=='{'||wholeWord[wholeWord.length()-word.length()-1]=='}'
                        ||wholeWord[wholeWord.length()-word.length()-1]=='('||wholeWord[wholeWord.length()-word.length()-1]==')'||wholeWord[word.length()]=='!')
                    {
                        check=true;
                    }
                }
            }
        }

    }

    if(!check)
    {
        stream.clear();
        stream.seekg(start, std::ios::beg);
    }

    return check;
}

string ExtractString(std::stringstream &stream, char delim, int outType)
{
    string value="";
    bool charOut=false, numOut=false, symOut=false;
    char letter;
    //bool first=true;

    if(outType==0)
    {

    }
    else if(outType==1)
    {
        charOut=true;
    }
    else if(outType==2)
    {
        numOut=true;
    }
    else if(outType==3)
    {
        charOut=true;
        numOut=true;
    }
    else if(outType==4)
    {
        symOut=true;
    }
    else if(outType==5)
    {
        charOut=true;
        symOut=true;
    }
    else if(outType==6)
    {
        numOut=true;
        symOut=true;
    }
    else
    {
        charOut=true;
        numOut=true;
        symOut=true;
    }

    while(stream&&(stream.peek()!=delim))
    {
        letter = stream.get();
        if(((letter>='A')&&(letter<='Z'))||((letter>='a')&&(letter<='z')))
        {
            if(charOut)
            {
                value+=letter;
                //first=true;
            }
            /*else if(first)
            {
                value+=' ';
                first=false;
            }*/
        }
        else if(((letter>='0')&&(letter<='9'))||(letter=='.')||(letter=='-'))
        {
            if(numOut)
            {
                value+=letter;
                //first=true;
            }
            /*else if(first)
            {
                value+=' ';
                first=false;
            }*/
        }
        else
        {
            if(symOut)
            {
                value+=letter;
                //first=true;
            }
            /*else if(first)
            {
                value+=' ';
                first=false;
            }*/
        }
    }
    return value;
}

string CreateMacroName(string geoFileName, string outDirName)
{
    if(geoFileName.back()!='/')
        geoFileName=geoFileName.substr(geoFileName.find_last_of('/')+1,geoFileName.find_last_of('.')-geoFileName.find_last_of('/')-1);
    else
        geoFileName="";

    return (outDirName+"DynMemCheck"+geoFileName+".txt");
}

void SetDataStream( string logFileName, std::stringstream& ss)
{
  std::ofstream out( logFileName.c_str() , std::ios::out | std::ios::trunc );
  if ( ss.good() )
  {
     ss.seekg( 0 , std::ios::end );
     int file_size = ss.tellg();
     ss.seekg( 0 , std::ios::beg );
     char* filedata = new char[ file_size ];

     while ( ss )
     {
        ss.read( filedata , file_size );
        if(!file_size)
        {
            cout << "\n #### Error the size of the stringstream is invalid ###" << endl;
            break;
        }
     }

     out.write(filedata, file_size);
     if (out.fail())
    {
        cout << endl << "writing the ascii data to the output file " << logFileName << " failed" << endl
             << " may not have permission to delete an older version of the file" << endl;
    }
     out.close();
     delete [] filedata;
  }
  else
  {
// found no data file
//                 set error bit to the stream
     ss.setstate( std::ios::badbit );

     cout << endl << "### failed to write to ascii file " << logFileName << " ###" << endl;
  }
   ss.str("");
}
