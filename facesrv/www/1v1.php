<html>
    <head>
        <script type="text/javascript" src="preview.js"></script>
    </head>

    <body>

<?php
include 'footer.php';
?>


        <form method='post' action='1v1.php' enctype='multipart/form-data'>
        <table>
            <tr>
                <td>
        <div id="preview0">
            <img id="imghead" width="100" height="100" border="0" src='../images/demo.jpg'>
        </div>
        </td>
        <td>
        <div id="preview1">
            <img id="imghead" width="100" height="100" border="0" src='../images/demo.jpg'>
        </div>
        </td>
        </tr>
        <tr><td>
            <input id='file1' type='file' name='userfile[]' onchange="previewImage(this, 'preview0')"/>
            </td>
            <td>
            <input type='file' name='userfile[]' onchange="previewImage(this, 'preview1')"/>
            </td>
            </tr>
        </table>
            <input type='submit' value='compare'/>
        <p>
     <?php
            define("UNIX_DOMAIN","/tmp/facesrv.s");
            if (!empty($_FILES['userfile'])) {
                $socket = socket_create(AF_UNIX, SOCK_STREAM, 0)
                          or die("Unable to create socket\n");
                $result = socket_connect($socket, UNIX_DOMAIN);
                if ($result >= 0) {
                    # echo '<p>file 0 --- ' . $_FILES['userfile']['name'][0] . '</p>';
                    # echo '<p>file 0 --- ' . $_FILES['userfile']['tmp_name'][0] . '</p>';
                    # echo '<p>file 1 --- ' . $_FILES['userfile']['tmp_name'][1] . '</p>';
                    # echo '<p>file all --- ' . $_FILES['userfile']['tmp_name'] . '</p>';
                    $in = "CMP0," . $_FILES['userfile']['tmp_name'][0] . ',' . $_FILES['userfile']['tmp_name'][1];     
                    # echo '<p>in --- ' . $in . '</p>';
                    if(socket_write($socket, $in, strlen($in))) {  
                        $buf = socket_read($socket, 1024);
                        echo 'result: ' . $buf;
                    }
                    else {
                        echo "socket_write() failed: reason: " . socket_strerror($socket) . "\n";  
                    }
                }
                else {
                    echo "socket_connect() failed.\nReason: ($result) " . socket_strerror($result) . "\n"; 
                }
                socket_close($socket);  
            }
            else {
                echo "no files";
            }
        ?>
        </p>
    </form>
</body>
</html>
