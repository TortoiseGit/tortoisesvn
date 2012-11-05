<?php

// Specify the directory where to save error reports
$file_root = "/home/crashrpt/crash_reports/";

// This is to avoid PHP warning
date_default_timezone_set('UTC');

// Writes error code and text message and then exits
function done($return_status, $message)
{
  // Write HTTP responce code
  header("HTTP/1.0 ".$return_status." ".$message);
  // Write HTTP responce body (for backwards compatibility)
  echo $return_status." ".$message;
  exit(0);
}

// Checks that text fild doesn't contain inacceptable symbols
function checkOK($field)
{
  if (stristr($field, "\\r") || stristr($field, "\\n"))
  {
    done(450, "Invalid input parameter.");
  }
}

$md5_hash = "";    // MD5 hash for error report ZIP
$file_name = "";   // Destination file name
$crash_guid = "";  // Crash GUID

// Check that MD5 hash exists
if(!isset($_POST['md5']))
{
  done(450, "MD5 hash is missing.");
}

// Get MD5 hash
$md5_hash = $_POST['md5'];
checkOK($md5_hash);
if(strlen($md5_hash)!=32)
{
  done(450, "MD5 hash value has wrong length.");
}

// Get CrashGUID
if(array_key_exists("crashguid", $_POST))
{
  $crash_guid = $_POST["crashguid"];
  checkOK($crash_guid);
  if(strlen($crash_guid)!=36)
  {
    done(450, "Crash GUID has wrong length.");
  }
}

// Get file attachment
if(array_key_exists("crashrpt", $_FILES))
{
  // Check upload error code
  $error_code = $_FILES["crashrpt"]["error"];
  if($error_code!=0)
  {
    done(450, "File upload failed with code $error_code.");
  }

  // Get temporary name uploaded file is stored currently
  $tmp_file_name = $_FILES["crashrpt"]["tmp_name"];
  checkOK($tmp_file_name);

  // Check that uploaded file data have correct MD5 hash
  $my_md5_hash = strtolower(md5_file($tmp_file_name));
  $their_md5_hash = strtolower($md5_hash);
  if($my_md5_hash!=$their_md5_hash)
  {
    done(451, "MD5 hash is invalid (yours is ".$their_md5_hash.", but mine is ".$my_md5_hash.")");
  }

  if(!empty($crash_guid))
  {
    // If crash GUID presents, use it as file name
    $file_name = $file_root.$crash_guid.".zip";
  }
  else
  {
    // Generate random file name
    $date_time = date(DATE_RFC822);
    $file_name = $file_root.md5(md5_file($tmp_file_name).$date_time).".zip";
  }

  // Move uploaded file to an appropriate directory
  if(!move_uploaded_file($tmp_file_name, $file_name))
  {
    done(452, "Couldn't save data to local storage");
  }
}
else
{
  // Assume legacy way is used
  if(!isset($_POST["crashrpt"]))
  {
    done(450, "Error report data is missing.");
  }

  // Get BASE64-encoded file data
  $encoded_data = $_POST["crashrpt"];

  // Decode file data
  $file_data = base64_decode($encoded_data);

  // Check that decoded data have correct MD5 hash
  $my_md5_hash = strtolower(md5($file_data));
  $their_md5_hash = strtolower($md5_hash);
  if($my_md5_hash!=$their_md5_hash)
  {
    done(451, "MD5 hash is invalid (yours is ".$their_md5_hash.", but mine is ".$my_md5_hash.")");
  }

  if(!empty($crash_guid))
  {
    // If crash GUID presents, use it as file name
    $file_name = $file_root.$crash_guid.".zip";
  }
  else
  {
    // Generate random file name
    $date_time = date(DATE_RFC822);
    $file_name = $file_root.md5($file_data.$date_time).".zip";
  }

  // Write decoded data to file
  $f = fopen($file_name, "w");
  if($f==FALSE)
  {
    done(452, "Couldn't save data to local storage");
  }

  fwrite($f, $file_data);
  fclose($f);
}

// OK.
done(200, "Success.");

?>
