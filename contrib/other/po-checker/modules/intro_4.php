<h1>Precommit checker</h1>
<p>You can upload file for check before you make actual commit, so you can fix errors in one commit.</p>
<p>Make sure you are going to check translation against proper branch.</p>
 <form enctype="multipart/form-data" action="index.php" method="post">
 <input type="hidden" name="MAX_FILE_SIZE" value="2048000" />
 Choose a file to upload: <input name="uploadedfile" type="file" /><br />
 <input type="submit" value="Upload File" />
 </form>
<?php


if (isset($_FILES['uploadedfile']['name'])) {
	$baseName=$_FILES['uploadedfile']['name'];
	$targetPath="../temp/";
	$targetName = $targetPth . basename( $baseName) . ".tmp" . session_id();

	if (move_uploaded_file($baseName, $targetName)) {
		$uploadresult="<br />The file" . basename( $baseName ) . " has been uploaded";
		if (preg_match_all("/(^[a-zA-Z]*)_([a-zA-Z_]*)\.(.*)/", $baseName, $matches)) {
			echo "<pre>";
//			var_dump($matches);
			echo "</pre>";
			$forceCode=$matches[2][0];
			$lang=$forceCode;
			$forcePoFileName=$target_path;
			$forceMode='g';
			echo "baseName=$baseName";
			if (strpos($baseName, "TortoiseSVN")!==false) {
				echo "SVN";
				$forceMode='s';
			} else if (strpos($baseName, "TortoiseMerge")!==false) {
				echo "merge";
				$forceMode='m';
			}
		}
		echo "<hr />forceCode : $forceCode <hr />";
	} else {
		$uploadError="<br />There was an error uploading file, please try again!";
	}
}



	echo $uploadresult;
	echo $uploadError;
//php?>
 <br />
 <br />

<hr />