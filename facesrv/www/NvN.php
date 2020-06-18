<html>
    <head>
        <script type="text/javascript" src="preview.js"></script>
    </head>

    <body>
<?php
include 'footer.php';
?>
        <form method='post' action='NvN.php' enctype='multipart/form-data'>
    <p>
     <?php
        define("UNIX_DOMAIN","/tmp/facesrv.s");
        $response = '';
        $socket = socket_create(AF_UNIX, SOCK_STREAM, 0)
            or die("Unable to create socket\n");
        $result = socket_connect($socket, UNIX_DOMAIN);
        if ($result >= 0) {
            $num = 0;
            #$in = 'CMP1,/home/hehj/workspace/facesrv/face/bin/photos/feature.idx,/home/hehj/workspace/facesrv/face/bin/photos/feature.idx';     
            $in='CMP1,/srv/www/mydb/feature.idx,/srv/www/mydb/feature.idx';
            if(socket_write($socket, $in, strlen($in))) {  
                echo '<table border="1"><tr><td>sn</td><td>score</td><td>file1</td><td>file2</td></tr>';
                while (false != ($buf = socket_read($socket, 1024))) {
                    if ($buf == "") continue;
                    $response = $response . $buf;
                    $rs = explode(',', $response); 
                    $count = count($rs);
                    for ($i = 0; $i <= $count-2; $i++) {
                        $val = $rs[$i];
                        if ($val == "") break;
                        $rec = explode('|', $val); 
                        echo '<tr><td>' . $num . '</td><td>' . $rec[0] . '</td><td>' . $rec[1] . '</td><td>' . $rec[2] . '</td></tr>';
                        $num++;
                    }
                    # echo '<tr><td>' . $count . '</td><td>' . $response . '</td><td>' . $rs[$count-1] . '</td><td>x</td></tr>';
                    $response = $rs[$count-1];
                }
                echo '</table>';
            }
            else {
                echo "socket_write() failed: reason: " . socket_strerror($socket) . "\n";  
            }
        }
        else {
            echo "socket_connect() failed.\nReason: ($result) " . socket_strerror($result) . "\n"; 
        }
        socket_close($socket);  
?>
        </p>
    </form>

</body>
</html>
