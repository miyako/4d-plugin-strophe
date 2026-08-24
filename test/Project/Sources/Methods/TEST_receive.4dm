//%attributes = {}
$params:=New object:C1471

$params.jid:="jimmy.kimmel@localhost/m"
$params.password:="abc"
$params.host:="localhost"

$status:=xmpp connect ($params)