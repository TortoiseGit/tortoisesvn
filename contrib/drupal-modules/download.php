<?php
$v['release']=variable_get('tsvn_version', '');
$v['build']=variable_get('tsvn_build', '');
$v['svnver']=variable_get('tsvn_svnlib', '');
$v['sf_release_id']=variable_get('tsvn_sf_release_binary', '');
$v['sf_project']=variable_get('tsvn_sf_project', '');
$v['url1']=variable_get('tsvn_sf_prefix', '');
$v['url2']=variable_get('tsvn_sf_append', '');

function t_print_changelog($v)
{
$t_ln="http://sourceforge.net/project/shownotes.php?release_id=".$v['sf_release_id'];
echo "<a href=\"$t_ln\">changelog</a>.";
}

function t_print_installer($v)
{
$t_ln="TortoiseSVN-".$v['release'].".".$v['build']."-svn-".$v['svnver'].".msi";
echo "Installer: <a href=\"";
echo $v['url1'].$t_ln.$v['url2'];
echo "\">".$t_ln."</a>";
}

function t_print_checksum($v)
{
$t_ln="TortoiseSVN-".$v['release'].".".$v['build']."-svn-".$v['svnver'].".md5";
echo "MD5 checksum for installer: <a href=\"";
echo $v['url1'].$t_ln.$v['url2'];
echo "\">".$t_ln."</a>";
}

?>

<h1>The latest Version is <?php echo $v['release'] ?>.</h1>

<ul>
<li>An overall look at what's in this version is available from the <a href="http://tortoisesvn.tigris.org/tsvn_1.3_releasenotes.html">Release Notes</a></li>
<li>For detailed info on what's new, read the <?php t_print_changelog($v); ?></li>
<li><?php t_print_installer($v); ?></li>
<li><?php t_print_checksum($v); ?></li>
<li>Language packs are available from the <a href="<?php print(url('translation_status')); ?>">translation status</a> page.</li>
<li>Spellchecker modules for several languages can be downloaded from the <a href="https://sourceforge.net/project/showfiles.php?group_id=<?php print $v['sf_project']; ?>&package_id=166583&release_id=364070">files section</a>.</li>
</ul>

<h1>Older Releases</h1>
<ul>
<li>Older releases are available from the <a href="http://sourceforge.net/project/showfiles.php?group_id=<?php print $v['sf_project']; ?>">Sourceforge files</a> section.</li>
</ul>

<h1>Nightly Builds</h1>
<ul>
<li><a href="http://mapcar.org/tsvn-snapshots/latest/">Nightly builds</a> are available too. These are for testing only. Please read the <a href="http://mapcar.org/tsvn-snapshots/latest/readme.txt">Readme.txt</a> first.</li>

<!--break-->
