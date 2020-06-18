<html>
    <head>
        <script type="text/javascript" src="preview.js"></script>
<script type="text/javascript">
window.onload = window_onload;
function window_onload() {
    var img = document.getElementById("imghead");
    var topmost = document.getElementById("topmost");
    if (topmost)
        img.src = topmost.src;
}
</script>

    </head>

    <body >
<?php
include 'footer.php';
?>


        <div id="preview0">
            <img id="imghead" width="100" height="100" border="0" src='../images/demo.jpg'>
        </div>

        <form method='post' action='1vN.php' enctype='multipart/form-data'>
        <table>
            <tr>
                <td>
                </td>
        </tr>
        <tr><td>
            <input id='file1' type='file' name='userfile' onchange="previewImage(this, 'preview0')"/>
            </td>
            </tr>
        </table>
            <input type='submit' value='compare'/>
        <p>
     <?php
            define("UNIX_DOMAIN","/tmp/facesrv.s");
            $response = '';
            $topmost = false;
            if (!empty($_FILES['userfile'])) {
                $socket = socket_create(AF_UNIX, SOCK_STREAM, 0)
                          or die("Unable to create socket\n");
                $result = socket_connect($socket, UNIX_DOMAIN);
                if ($result >= 0) {
                    # echo '<p>file 0 --- ' . $_FILES['userfile']['name'][0] . '</p>';
                    # echo '<p>file 0 --- ' . $_FILES['userfile']['tmp_name'][0] . '</p>';
                    # echo '<p>file 1 --- ' . $_FILES['userfile']['tmp_name'][1] . '</p>';
                    # echo '<p>file all --- ' . $_FILES['userfile']['tmp_name'] . '</p>';
                    $in = "IDEN," . $_FILES['userfile']['tmp_name'];     
                    # echo '<p>in --- ' . $in . '</p>';
                    if(socket_write($socket, $in, strlen($in))) {  
                        echo '<table border="1"><tr><td>score</td><td>file</td></tr>';
                        while (false != ($buf = socket_read($socket, 1024))) {
                            if ($buf == "") continue;
                            $response = $response . $buf;
                            $rs = explode(',', $response); 
                            $count = count($rs);
                            for ($i = 0; $i <= $count-2; $i++) {
                                $val = $rs[$i]; 
                                if ($val == "") break;
                                $rec = explode('|', $val);
                                echo '<tr><td>' . $rec[0] . '</td><td>' . $rec[1] . '</td></tr>';
                                if ($topmost == false)
                                    $topmost = $rec[1];
                                $num++;
                            }       
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
                if ($topmost != false) {
                    #$topmost = str_replace('/srv/www/', '', $topmost);
                    #echo '<script type="text/javascript">document.getElementById(imghead).src="' . $topmost . '";</script>';
                    echo '<img hidden="true" id="topmost" width="100" height="100" border="0" src="mydb/' . $topmost . '">';
                } 
            }
            else {
                echo "no files";
            }
        ?>
        </p>
    </form>

</body>
</html>
