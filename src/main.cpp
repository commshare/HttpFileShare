
#include "mongoose.h"
#include "sqlite3/sqlite3.h"
#include "window.h"

#include <time.h>
#include <iostream>
#include <sstream>

#include <QTime>
#include <QDir>
#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QTextCodec>
#include <QSettings>
#include <QUrl>
#include <QDate>
#include <QMessageBox>


using namespace std;
sqlite3 * pDB;



//char* U2G(const char* utf8)//UTF-8到GB2312的转换
//{
//	int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
//	wchar_t* wstr = new wchar_t[len+1];
//	memset(wstr, 0, len+1);
//	MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wstr, len);
//	len = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
//	char* str = new char[len+1];
//	memset(str, 0, len+1);
//	WideCharToMultiByte(CP_ACP, 0, wstr, -1, str, len, NULL, NULL);
//	if(wstr) delete[] wstr;
//	return str;
//}
//
//char* G2U(const char* gb2312)//GB2312到UTF-8的转换
//{
//	int len = MultiByteToWideChar(CP_ACP, 0, gb2312, -1, NULL, 0);
//	wchar_t* wstr = new wchar_t[len+1];
//	memset(wstr, 0, len+1);
//	MultiByteToWideChar(CP_ACP, 0, gb2312, -1, wstr, len);
//	len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
//	char* str = new char[len+1];
//	memset(str, 0, len+1);
//	WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, NULL, NULL);
//	if(wstr) delete[] wstr;
//	return str;
//}


int createTable()
{
	string createTab = "CREATE TABLE http_log (filename TEXT, filepath TEXT, id INTERGER PRIMARY KEY, sender TEXT, time TEXT);";
	char* errMsg;

	int res = sqlite3_exec(pDB , createTab.c_str() ,0 ,0, &errMsg);
	if (res != SQLITE_OK)
	{
		std::cout << "执行创建table的SQL 出错." << errMsg << std::endl;
		return -1;
	}
	else
	{
		std::cout << "创建table的SQL成功执行."<< std::endl;
	}
	return 0;
}

int queryByMD5(QString& filename_, QString& filepath_, QString MD5) 
{
	QString strsql = QString("SELECT filename , filepath FROM http_log WHERE MD5=")+("\"");
	strsql.append(MD5+"\"");
	sqlite3_stmt * stmt;
	sqlite3_prepare(pDB,
		strsql.toStdString().c_str(),
		-1,&stmt,0);
	int rc = sqlite3_step(stmt);
	if (rc != SQLITE_ROW)
	{
		std::cout << "quuryByMD5 执行出错."<< std::endl;
		return -1;
	}
	while( rc == SQLITE_ROW )
	{
		filename_ = QObject::trUtf8((const char*) sqlite3_column_text(stmt,0));
		filepath_ = QObject::trUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt,1)));

		rc=sqlite3_step(stmt);
	}
	sqlite3_finalize(stmt);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////
void getFile(struct mg_connection *conn)
{
	QString uri(conn->uri+1);
	QString queryString (conn->uri);

	QString md5 = queryString.mid(queryString.indexOf("md5=",Qt::CaseInsensitive)+4);
	QString fileName, filePath;
	QString currentDir = QDir::currentPath();//QApplication::applicationDirPath();
	QString absolutePath;
	qDebug()<<currentDir;
	absolutePath = currentDir+"/" + uri;

	QFile  file(absolutePath);
	if(!file.exists()) // if file not exist show error to client and retrun 1
	{
		qDebug()<<"file not found"<<endl;
		mg_printf(conn, "HTTP/1.1 %d %s\r\n"
			"Connection: %s\r\n\r\n", 404, "Not Found",
			"closed");
		return ;
	}

	QString updateTable = "UPDATE http_log set downloaded='1' where filepath=\'"+uri+"\'";
	char* errMsg;
	int res = sqlite3_exec(pDB , updateTable.toStdString().c_str() ,0 ,0, &errMsg);
	if (res != SQLITE_OK)
	{
		std::cout << "更新table的SQL 出错." << errMsg << std::endl;
		return ;
	}
	else
	{
		std::cout << "更新table的SQL成功执行."<< std::endl;
	}

}

/////////////////////////////////////////////////////////////////////////////////////////////


void showUploadForm(struct mg_connection *conn)
{
	// Show HTML form. Make sure it has enctype="multipart/form-data" attr.
	static const char *html_form = 
		"<html><body>Upload example."
		"<form method=\"POST\" action=\"/postfile\" "
		"  enctype=\"multipart/form-data\">"
		"<input type=\"file\" name=\"file\" /> <br/>"
		"<input type=\"submit\" value=\"Upload\" />"
		"</form></body></html>";

	QFile indexFile("./index.html");
	QString index_string;

	if(indexFile.exists())
	{
		if(!indexFile.open(QIODevice::ReadOnly))
			return;
		mg_printf(conn, "HTTP/1.1 200 OK\r\n"
			"Content-Length: %d\r\n"
			"Content-Type: text/html\r\n\r\n%s",
			(int) indexFile.size(), indexFile.readAll().data());
		indexFile.close();
		return ;
	}else
	mg_printf(conn, "HTTP/1.1 200 OK\r\n"
		"Content-Length: %d\r\n"
		"Content-Type: text/html\r\n\r\n%s",
		(int) strlen(html_form), html_form);
}

//////////////////////////////////////////////////////////////////////////////////////////
int insertRecord(QString &md5, QString &filepath, QString &filename,QString type="file", QString sender="source", QString receiver="target")
{
	if (md5.isEmpty()      || 
		filepath.isEmpty() ||
		filename.isEmpty() )
	{
		return -1; //检查输入是否为空
	}

	QString timeString;
	QTime t = QTime::currentTime();
	QDate date = QDate::currentDate();
	timeString = (date.toString("yyyy-MM-dd/")).append(t.toString("hh:mm:ss"));

	QTextCodec::setCodecForCStrings(QTextCodec::codecForName("system"));//这个非常重要，没有这个，下面的filename.toUtf8()就会出错
	char* errMsg;

	QString downloaded ="0";
	QString strsql = QString("insert into http_log values( ");  
							//insert into http_log values("md5", "receiver", "filename", "filepath", "sender", "time")
	strsql.append("\""+ type+ "\", "); //type
	strsql.append("\""+ downloaded+ "\", "); //downloaded
	strsql.append("\""+ md5+ "\", ");
	strsql.append("\""+ receiver+ "\", ");
	strsql.append("\""+ filename.toUtf8()+ "\", ");
	strsql.append("\""+ filepath.toUtf8()+ ("\", "));
	strsql.append("\""+ (sender)+ ("\", "));
	strsql.append(("\"")+ (timeString)+ ("\");"));

	int res = sqlite3_exec(pDB,"begin transaction;",0,0, &errMsg);
	res = sqlite3_exec(pDB,strsql.toStdString().c_str(),0,0, &errMsg); //insert
	if (res != SQLITE_OK)
	{
		std::cout << "insertRecord error." << errMsg << std::endl;
		return -2;//插入出错
	}
	res = sqlite3_exec(pDB,"commit transaction;",0,0, &errMsg);
	std::cout<<"file information of "<<filename.toStdString()<<" is inserted into the database."<< std::endl;
	return 0;
}


char *rand_str(char *str,const int len)  //生成随机数
{
	int i;
	for(i=0;i<len-1;i++)
		str[i]='A'+rand()%26;
	str[i]='\0';
	return str;
}

void postFile(struct mg_connection *conn) 
{
	QString  md5, filePath, fileName;
	QString receiver,sender,type ;
	char buf[33], randomString[33];

	const char *data;
	int data_len;
	char var_name[1024], file_name[1024];

	QString refer(mg_get_header(conn, "Referer"));
	QUrl referUrl(refer);
	QString dirPath = referUrl.path();
	QString dataDirString;
	QString workingDir = QDir::currentPath();
	QString pathInDB ;
	if(dirPath != "/" || !dirPath.isEmpty())
	{
		dataDirString = workingDir + dirPath;
		if (mg_parse_multipart(conn->content, conn->content_len,
			var_name, sizeof(var_name),
			file_name, sizeof(file_name),
			&data, &data_len) > 0) 
		{
			fileName = QObject::trUtf8(file_name);
			if(fileName.contains('\\'))
				fileName = fileName.mid(fileName.lastIndexOf('\\')+1);// iexplorer upload file with filename as "c:\documents\user\desktop\ss.txt"

			filePath = dataDirString+ "/"+  fileName;
			pathInDB = dirPath+ "/"+ fileName;
			QFile file(filePath);
			if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
			{
				file.write(data,data_len);
				file.flush();
				file.close();
			}
			cout<<"file upload sucess: "<<filePath.toStdString()<<endl;
		}
		QString URL;
		QString host = QString(mg_get_header(conn,"Host"));
		URL = "http://"+ host +"/"+ pathInDB;

		mg_printf_data(conn, "<html><body>File uploaded Sucess as: [<A href=\"%s\" target=\"_blank\">%s</A>]</body></html>",
			URL.toStdString().c_str(),
			fileName.toStdString().c_str());
		return;
	}
	else dataDirString = workingDir + "/data";

	QString date = QDate::currentDate().toString("yyyyMMdd");
	QDir qdir;	qdir.mkpath(dataDirString+"/"+date); 

	QString time = QTime::currentTime().toString("hhmmsszzz");
	if (mg_parse_multipart(conn->content, conn->content_len,
		var_name, sizeof(var_name),
		file_name, sizeof(file_name),
		&data, &data_len) > 0) 
	{
		fileName = QObject::tr(file_name);
		if(fileName.contains('\\'))
			fileName = fileName.mid(fileName.lastIndexOf('\\')+1);// iexplorer upload file with filename as "c:\documents\user\desktop\ss.txt"

		filePath = dataDirString +"/"+ date+ "/"+ time+"_"+ fileName;
		QString pathInDB = "data/"+date+"/"+ time+"_"+ fileName;

		QFile file(filePath);
		if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
		{
			file.write(data,data_len);
			file.flush();
			file.close();
		}
		cout<<"file upload sucess: "<<filePath.toStdString()<<endl;
		receiver = QString(mg_get_header(conn, "receiver"));
		sender = QString(mg_get_header(conn, "sender"));
		type = QString(mg_get_header(conn, "type"));
		if(receiver.isEmpty()) receiver = "target";
		if(sender.isEmpty()) sender="source";
		if(type.isEmpty()) type="file";

		rand_str(randomString, sizeof(randomString));
		mg_md5(buf, randomString,NULL); //必须要以NULL结尾
		md5 = QString(buf);
		insertRecord(md5, pathInDB, fileName, type, sender, receiver); //sender and receiver is default set to "source" and "target"
		QString URL;
		QString host = QString(mg_get_header(conn,"Host"));
		URL = "http://"+ host +"/"+ pathInDB;//"/getfile?md5=" + md5;  //http://host/getfile?md5=XXXX  or getfile?md5=XXXX

		/*mg_printf(conn, "HTTP/1.1 200 OK\r\n"
			"Content-Length: %d\r\n"
			"Connection: Keep-Alive\r\n"
			"Content-Type: text/html\r\n\r\n%s",
			URL.toStdString().size(),
			URL.toStdString().c_str());*/

	//mg_printf(conn, "HTTP/1.1 200 OK\r\n"
	//		"Connection: close\r\n"    //keep-alive
	//		"Content-Length: 300\r\n"
	//		"fileurl: %s\r\n\r\n",
	//		URL.toStdString().c_str());

	mg_printf_data(conn, "<html><body>File saved as: [<A href=\"%s\" target=\"_blank\">%s</A>]</body></html>",
		URL.toStdString().c_str(),
		fileName.toStdString().c_str());
				
	}
	else 
		mg_printf(conn, "%s", "HTTP/1.0 200 OK\r\n\r\nNo files sent");
}

///////////////////////////////////////////////////////////////////////////////////////////

void queryFileError(struct mg_connection *conn, const QString & errorString)
{
	QString html_queryError=	"<html><body>";
	html_queryError.append(errorString);
	html_queryError.append("</body></html>");
	mg_printf(conn, "HTTP/1.1 200 OK\r\n"
		"Content-Length: %d\r\n"
		"Content-Type: text/html\r\n\r\n%s",
		html_queryError.length(), html_queryError.toStdString().c_str());

}
void qureryFile(struct mg_connection *conn) 
{
	QString queryString= QString::fromAscii(conn->query_string);
	QString urlString = QString("http://127.0.0.1/queryfile?")+queryString;
	QUrl url(urlString);
	QString userName=url.queryItemValue("receiver");
	QString downloaded = url.queryItemValue("downloaded");

	if (downloaded.isEmpty())
		downloaded = "0";

	QString fileName, md5;
	QString strsql = QString("SELECT filename , md5, sender, receiver, downloaded,type, time, filepath  FROM http_log ");
	if(!userName.isEmpty())
	{
		strsql.append("WHERE receiver=\"");
		strsql.append(userName+ "\"");
		if (downloaded != "2") //downloaded=2 输出全部文件
		{
			strsql.append(" AND ");
			strsql.append("downloaded=\"");
			strsql.append(downloaded);
			strsql.append("\"");
		}
	}

	if(queryString.isEmpty()) //空串查找全部内容
		strsql = QString("SELECT filename , md5, sender, receiver, downloaded,type, time,filepath  FROM http_log ");

	sqlite3_stmt * stmt;
	sqlite3_prepare(pDB, strsql.toStdString().c_str(), -1,&stmt,0);
	int rc = sqlite3_step(stmt);
	if (rc != SQLITE_ROW && rc!=SQLITE_DONE)
	{
		std::cout << "queryFile By User Name 执行出错."<< std::endl;
		queryFileError(conn, QString("queryFile error: sql error."));
		return ;
	}
	QStringList fileList, md5List;
	QStringList senderList, receiveList, downloadedList, timeList,typeList, pathList;
	while( rc == SQLITE_ROW )
	{
		fileName = QObject::trUtf8((const char*) sqlite3_column_text(stmt,0));
		md5 = QObject::trUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt,1)));
		QTextCodec::setCodecForCStrings(QTextCodec::codecForName("GBK"));
		fileList<<fileName;
		md5List<<md5;

		senderList<<QObject::trUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt,2)));
		receiveList<<QObject::trUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt,3)));
		downloadedList<<QObject::trUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt,4)));
		typeList<<QObject::trUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt,5)));
		timeList<<QObject::trUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt,6)));
		pathList<<QObject::trUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt,7)));

		rc=sqlite3_step(stmt);
	}
	sqlite3_finalize(stmt);

	QString html_queryResult=QObject::tr("<html><body><table>\
										 <tr><th>sender</th><th>receiver</th><th>name</th><th>downloaded</th><th>type</th><th>time</th></tr>");
	for(int i=0; i<fileList.size(); i++)
	{
		QString URL;
		QString host = QString(mg_get_header(conn,"Host"));
		//URL = "http://"+ host + "/getfile?md5=" + md5List.at(i);  //http://host/getfile?md5=XXXX
		URL = "http://"+ host + "/" + pathList.at(i);  //"http://host/20120302/121103_upfile.zip"
		html_queryResult.append("<tr>");
		html_queryResult.append("<th>"+ senderList.at(i)+ "</th>");
		html_queryResult.append("<th>"+ receiveList.at(i)+ "</th>");

		html_queryResult.append("<th><A href=\"");
		html_queryResult.append(URL +"\">");
		html_queryResult.append(fileList.at(i)+"</A></th>");

		html_queryResult.append("<th>"+ downloadedList.at(i)+ "</th>");
		html_queryResult.append("<th>"+ typeList.at(i)+ "</th>");
		html_queryResult.append("<th>"+ timeList.at(i)+ "</th>");

		html_queryResult.append("</tr>");
	}

	html_queryResult.append("</table></body></html>");
	mg_printf(conn, "HTTP/1.1 200 OK\r\n"
		"Content-Length: %d\r\n"
		"Connection: Keep-Alive\r\n"
		"Content-Type: text/html\r\n\r\n%s",
		html_queryResult.toStdString().size(),
		html_queryResult.toStdString().c_str());
}


///////////////////////////////////////////////////////////////////////////////////////////

static int event_handler(struct mg_connection *conn, enum mg_event ev) 
{

	if (ev == MG_REQUEST) 
	{
		if (!strcmp(conn->uri, "/postfile")) // post file
		{
			postFile(conn);
			return 1;
		}
		else if(strstr(conn->uri, "/getfile"))  //get file
		{
			//getFile(conn);
			return 1;
		}
		else if(strstr(conn->uri, "/queryfile"))  //get file
		{
			qureryFile(conn);
			return 1;
		}
		else if(!strcmp(conn->uri, "") || !strcmp(conn->uri, "/"))   //request url is null return html for broswer
		{
			showUploadForm(conn);
			return 1;
		}
		getFile(conn);
		return MG_FALSE;
	} else if (ev == MG_AUTH) 
	{
		return MG_TRUE;
	} 

	return MG_FALSE;
	
}



////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
	QTextCodec::setCodecForTr(QTextCodec::codecForName("GBK"));//必须要有的，上面用到了
	QTextCodec::setCodecForCStrings(QTextCodec::codecForName("GBK"));
	srand(time(0));

    QApplication app(argc,argv);

	QString db_file_name = "./log.sqlite";
    QString programDir = QCoreApplication::applicationDirPath();
    QFile dbfile(programDir+"/"+ db_file_name);
	if(!dbfile.exists())
	{
		QString errorMsg = "Can't find database file :" + db_file_name;
		QMessageBox::warning(NULL, "Error", errorMsg);
		return -1;
	}
	int res = sqlite3_open("./log.sqlite", &pDB);
	if( res )
	{
		std::cout << "Can't open database: "<< sqlite3_errmsg(pDB);
		sqlite3_close(pDB);
		return -1;
	}

    //Window window;
    //window.show();
	QString port, document_root;
	QSettings settings("httpServerSettings.ini", QSettings::IniFormat);
	settings.beginGroup("httpFileServer");
	port = settings.value("port","8080").toString();
	document_root = settings.value("document_root","./").toString();
	settings.endGroup();

	char port_c[10] ,document_root_c[256];
    strcpy(port_c,port.toStdString().c_str() );
    strcpy(document_root_c, document_root.toStdString().c_str());

	struct mg_server *server;
	server = mg_create_server(NULL, event_handler);
	mg_set_option(server, "listening_port", port_c);
	mg_set_option(server, "document_root", "./");	

	printf("Server started on port %s\n", mg_get_option(server, "listening_port"));
	for (;;) {
		mg_poll_server(server, 1000);
	}
	mg_destroy_server(&server);

	return 0;
}
