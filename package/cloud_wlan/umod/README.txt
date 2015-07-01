1.　cloud wlan user process need support: sqlite3 and stat

Base system
    busybox
        Coreutils
            <*>stat


Libraries
    database
        -*- libsqlite3.


Utilities
    database
        <*> sqlite3-cli
		
2.　最好只发主域名不带参数		
关于接收的url格式如下：
	http://www.chinanews.com:8080/gj/2014/04-16/6069354.shtml
	http://www.chinanews.com/gj/2014/04-16/6069354.shtml
	www.chinanews.com:8080/gj/2014/04-16/6069354.shtml
	www.chinanews.com/gj/2014/04-16/6069354.shtml

	解析不了的域名如下
	http://openapi.qzone.qq.com/oauth/show?which=Login&display=pc&client_id=101032552&redirect_uri=http://www.zoomnetwork.com.cn/portal/afterQQLogin.do&response_type=code&state=173cc01df2c52b372f1b2c7d88771973&scope=get_user_info,add_top