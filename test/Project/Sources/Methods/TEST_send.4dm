//%attributes = {}
$params:=New object:C1471

$params.jid:="jimmy.fallon@localhost/m"
$params.password:="nbc"
$params.host:="localhost"

  //message stanza
$message:=New object:C1471(\
"body";"Hello!";\
"type";"chat";\
"id";Generate UUID:C1066;\
"to";"jimmy.kimmel@localhost")

$status:=xmpp send message ($params;$message)