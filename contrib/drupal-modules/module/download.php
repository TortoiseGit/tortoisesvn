<?php

include("/var/www/vhosts/default/htdocs/modules/tortoisesvn/trans_data_branch.inc");
include("/var/www/vhosts/default/htdocs/modules/tortoisesvn/trans_countries.inc");

$v['release']=variable_get('tsvn_version', '');
$v['build']=variable_get('tsvn_build', '');
$v['build_x64']=variable_get('tsvn_build_x64', '');
$v['svnver']=variable_get('tsvn_svnlib', '');
$v['sf_release_id']=variable_get('tsvn_sf_release_binary', '');
$v['sf_project']=variable_get('tsvn_sf_project', '');
$v['url1']=variable_get('tsvn_sf_prefix', '');
$v['url2']=variable_get('tsvn_sf_append', '');
$v['flagpath']="/flags/world.small/";

$w['w32']=$v['release'].".".$v['build']."-win32"; 
$w['w32wrong']=$v['release'].".".$v['build'].""; 
$w['x64']=$v['release'].".".$v['build_x64']."-x64"; 

if (!function_exists('get_changelog')) {
function get_changelog($v)
{
$t_ln="http://sourceforge.net/project/shownotes.php?release_id=".$v['sf_release_id'];
return "<a href=\"$t_ln\">changelog</a>";
}
}

if (!function_exists('get_installer')) {
function get_installer($v, $w)
{
$t_ln="TortoiseSVN-".$w."-svn-".$v['svnver'].".msi";
return "<a href=\"".$v['url1'].$t_ln.$v['url2']."\">$t_ln</a>" ;
}
}

if (!function_exists('get_checksum')) {
function get_checksum($v, $w)
{
$t_ln="TortoiseSVN-".$w."-svn-".$v['svnver'].".md5";
return "<a href=\"".$v['url1'].$t_ln.$v['url2']."\">$t_ln</a>";
}
}

if (!function_exists('get_langpack')) {
function get_langpack($l, $n, $v, $w)
{
$t_ln="LanguagePack-".$w."-".$l.".exe";
return "<a href=\"".$v['url1'].$t_ln.$v['url2']."\">$n</a>";
}
}

if (!function_exists('print_langpack')) {
function print_langpack($i, $postat, $v, $w)
{
  $infobits=$postat[1];
  $lang_cc=$postat[2];
  $lang_name=$postat[3];
  $flagimg=$v['flagpath'].$lang_cc.".png";
  $flagtag="<img src=\"$flagimg\" height=\"12\" width=\"18\" alt=\"$lang_name flag\"/>";
  
  $dlfile32=get_langpack($lang_cc, 'Setup', $v, $w['w32']);
  $dlfile64=get_langpack($lang_cc, 'Setup', $v, $w['x64']);
  if ( ($infobits & "001") <> "0") {
   $t_ln="SpellChecker_".$lang_cc.".exe";
   $dlfilechecker="<a href=\"".$v['url1'].$t_ln.$v['url2']."\">Spellchecker</a>";
  } else {
   $dlfilechecker="";
  }
  
  if ( ($infobits & "010") <> "0") {
   $t_ts="TortoiseSVN-".$v['release'].'-'.$lang_cc.".pdf";
   $dlmanTSVN="<a href=\"".$v['url1'].$t_ts.$v['url2']."\">TSVN</a>";
  } else {
   $dlmanTSVN="";
  }

  if ( ($infobits & "100") <> "0") {
   $t_tm="TortoiseMerge-".$v['release'].'-'.$lang_cc.".pdf";
   $dlmanTMerge="<a href=\"".$v['url1'].$t_tm.$v['url2']."\">TMerge</a>";
  } else {
   $dlmanTMerge="";
  }

  echo "<tr class=\"stat_ok\">";
  echo "<td>$i</td>";
  echo "<td>$flagtag&nbsp;$lang_name</td>";
  echo "<td>$dlfile32</td>";
  echo "<td>$dlfile64</td>";
  echo "<td>$dlfilechecker</td>";
  echo "<td>$dlmanTSVN</td>";
  echo "<td>$dlmanTMerge</td>";
  echo "</tr>";
}
}

//------------------------------------
//
// The program starts here
//

?>
<h1>The current version is <?php echo $v['release'] ?>.</h1>
<p>
For detailed info on what's new, read the <?php echo get_changelog($v); ?> and the <a href="http://tortoisesvn.tigris.org/tsvn_1.4_releasenotes.html">release notes</a>.
</p>
<p>
This page points to installers for 32 bit and 64 bit operating systems. Please make sure that you choose the right installer for your PC. Otherwise the setup will fail.
</p>
<p>
Note for x64 users: you can install both the 32 and 64-bit version side by side. This will enable the TortoiseSVN features also for 32-bit applications.
</p>

<div class="table">
<table>
<tr>
<td>&nbsp;</td>
<th colspan="2">Download Application</th>
</tr>
<tr>
<th>32 Bit</th>
<td><?php echo get_installer($v,$w['w32']) ?></td>
<td>Installer</td>
</tr>
<tr>
<td>&nbsp;</td>
<td><?php echo get_checksum($v,$w['w32']) ?></td>
<td>MD5 checksum</td>
</tr>
<tr>
<th>64 Bit</th>
<td><?php echo get_installer($v,$w['x64']) ?></td>
<td>Installer</td>
</tr>
<tr>
<td>&nbsp;</td>
<td><?php echo get_checksum($v,$w['x64']) ?></td>
<td>MD5 checksum</td>
</tr>
</table>
</div>

<br />
<script type="text/javascript"><!--
google_ad_client = "pub-0430507460695576";
/* 300x250, tsvn.net inPage */
google_ad_slot = "5167477883";
google_ad_width = 300;
google_ad_height = 250;
//-->
</script>
<script type="text/javascript"
src="http://pagead2.googlesyndication.com/pagead/show_ads.js">
</script>
<?php

// Merge translation and country information into one array
$TortoiseGUI = array_merge_recursive($countries, $TortoiseGUI);

// Convert Data into a list of columns
foreach ($TortoiseGUI as $key => $row) {
   $potfile[$key] = $row[0];
   $country[$key] = $row[3];
   $errors[$key] = $row[5];
   $total[$key] = $row[6];
   $transl[$key] = $row[7];
   $fuzzy[$key] = $row[8];
   $untrans[$key] = $row[9];
   $accel[$key] = $row[10];
   $name[$key] = $row[11];
   $fdate[$key] = $row[12];
}

// Add $TortoiseGUI as the last parameter, to sort by the common key
array_multisort($potfile, $country, $transl, $untrans, $fuzzy, $accel, $TortoiseGUI);

?>

<h2>Language packs</h2>
<div class="table">
<table class="translations">
<tr>
<th class="lang">&nbsp;</th>
<th class="lang">Country</th>
<th class="lang">32 Bit</th>
<th class="lang">64 Bit</th>
<th class="lang">Spellchecker</th>
<th class="lang" colspan="2">Separate manual (PDF)</th>
</tr>

<?php
  $i=0;
  foreach ($TortoiseGUI as $key => $postat)
    if (isset($postat[5]) && ($postat[0] == "1") ) {
      $i++;
      print_langpack($i, $postat, $v, $w);
    }
?>

</table>
</div>

<h1>Forthcoming Releases</h1>
<p>To find out what is happening with the project and when you can expect the next release, take a look at our <a href="/status">project status</a> page.</p>

<h1>Release Candidates</h1>
<p>We maintain ongoing <a href="http://sourceforge.net/project/showfiles.php?group_id=138498">Release Candidates</a>
<!--
<a href="http://nightlybuilds.tortoisesvn.net/1.4.x/">Release Candidates</a>
-->
as well. These contain the latest official release plus latest bugfixes. They are not built nightly, but on demand from the current release branch. If you find that a certain bug has been fixed and you do not want to wait until the next release, install one of these. You would also help us tremendously by installing and testing release candidates.
Please read <a href="http://nightlybuilds.tortoisesvn.net/1.4.x/Readme.txt">Readme.txt</a> first.</p>

<h1>Nightly Builds</h1>
<p><a href="http://nightlybuilds.tortoisesvn.net/latest/">Nightly Builds</a> are available too. They are built from the current development head and are for testing only. Please read <a href="http://nightlybuilds.tortoisesvn.net/latest/Readme.txt">Readme.txt</a> first.</p>

<h1>Older Releases</h1>
<p>Older releases are available from the <a href="http://sourceforge.net/project/showfiles.php?group_id=<?php print $v['sf_project']; ?>">Sourceforge files</a> section.</p>

<h1>Sourcecode</h1>
<p>TortoiseSVN is under the GPL license. That means you can get the whole sourcecode and build the program yourself.
<br />
The sourcecode is hosted on <a href="http://www.tigris.org">tigris.org</a> in our own Subversion repository. You can browse the sourcecode with your favorite webbrowser directly on the <a href="http://tortoisesvn.tigris.org/svn/tortoisesvn/">repository</a>. Use <em>guest</em> as the username and an empty password to log in.
<br />
If you have TortoiseSVN installed, you can check out the whole sourcecode by clicking on the tortoise icon below:
<br />
<a href="tsvn:http://tortoisesvn.tigris.org/svn/tortoisesvn/trunk"><img src="/files/TortoiseCheckout.png" alt="Tortoise Icon"/></a></p>

<script type="text/javascript"><!--
google_ad_client = "pub-0430507460695576";
/* 300x250, tsvn.net inPage */
google_ad_slot = "5167477883";
google_ad_width = 300;
google_ad_height = 250;
//-->
</script>
<script type="text/javascript"
src="http://pagead2.googlesyndication.com/pagead/show_ads.js">
</script>
<!--break-->
