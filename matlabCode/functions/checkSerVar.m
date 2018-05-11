function parsedVar=checkSerVar(comObj,tChar,vBuf)
    fprintf(comObj,[tChar '<']);
    tempBuf=fscanf(comObj);
    splitBuf=strsplit(tempBuf,',');
    if strcmp(splitBuf{1},'echo') && strcmp(splitBuf{2},tChar)
        parsedVar=str2num(splitBuf{3});
    else 
        parsedVar=[];
    end
end