<?php
    $dir=scandir(".");
    foreach ($dir as $file) {
        if (preg_match("/([^\.]*)\.png/", $file, $matches)) {
            $file=$matches[1];
            echo "<img src=\"$file.png\">$file<br><br>";
        }
    }
//php?>