import re
lines = open('gldef.in','r')
for line in lines:
    print line.replace("BEGIN ","").replace("END","").replace("PREFIX","exp").replace("SECOND","STDCALL") + "{"
    #print "{"
    funcname = line[line.index("PREFIX")+6:line.index("(")].split(" ")[0]
    #print 4*" "+"static ( *p"+funcname+" )("+line[line.index("(")+1:line.index(")")]+") = NULL;"
    #print 4*" "+"if ( !p"+funcname+" ) p"+funcname+" = GL_GetProcAddress(\""+funcname+"\");"
    args = ""
    first = True
    for arg in line[line.index("(")+1:line.index(")")].split(", "):
        if(arg != "void"):
            argsplit = re.split(' |\*', arg)
            #print argsplit[-1]
            if(not(first)):
                args = args + ", "
            first = False
            args = args + argsplit[-1]
    returnstr = "" if (line.split(' ')[1] == "void") else "return "
    print " "*4+returnstr+"ldrp"+funcname+" (" + args + ");"
    print "}"
    print ""