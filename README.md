

HttpFileShare


## what's this for?
 Usually http file share software just allow you download files ,but you can't upload files on to the server.
By this you can upload  and download files as you like.
This server is designed for some other usage. so there is something you don't need to share files,but it doesn't affect your usage.
May be it's not very easy to use , it's always updated.

## usage
Just build and run.

 1.To build ,youd need Qt develop environment. Qt creator or MS visual studio with Qt Visual Studio Add-in.
you can use Qt Creator open src/HttpFileShare.pro
the excutable file is generated in bin/
log.sqlite  is a sqlite database file. it store s all the information need by server.
SQLite Database Browser.exe is used to explorer log.sqlite ,you can modify the database as you need.
sqlite3.dll is needed to run program.
httpServerSettings.ini stores server port and  document root info. You can modify it as you need.
data/  files uploaded is stored in data folder.
index.html is used to upload files at start.

 2.run
after build ,the server.exe will be generated in bin/ ,just double click it to run. you can use iexplorer to access http://127.0.0.1:8080 to see the reslut.

## LOG
2014.4.6 add upload for everywhere.

## TODO
add GBK filename support.

