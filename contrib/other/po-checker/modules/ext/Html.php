<?php
define("NEW_LINE", "\r\n");
define("NEW_LINE_TAB", "\r\n\t");

define("HTML_SCRIPT_JAVASCRIPT", "text/javascript");

define("HTML_STYLE_CSS", "text/css");

define("HTML_ELEMENT_OPEN", 0);
define("HTML_ELEMENT_SELFCLOSE", 1);
define("HTML_ELEMENT_CLOSE", 2);
define("HTML_ELEMENT_SMARTCLOSE", 3);

class html
{

   // public property declaration
   public $type = "xhtml10t";
   public $skipDefault = false;
   public $checkRules = false;
   public $cssAsStyle = false;///< false -> css as link

   // private property declaration
   private $_header = array();
   private $_isHeaderSent = false;
   private $_isOpen = false;

   function __construct()
   {
   }

   function __destruct()
   {
      if($this->_isOpen)
      {
         echo $this->Close();
      }
   }

   // method declaration
   public function Close()
   {
      $ret = "";
      if( ! $this->_isOpen)
      {
         $ret .= $this->Open();
      }
      if( ! $this->_isHeaderSent)
      {
         $ret .= $this->RenderHead();
         $this->OpenBody();
      }
      $this->_isOpen = false;
      $ret .= NEW_LINE . "</body>";
      $ret .= NEW_LINE . "</html>";
      return $ret;
   }
   // method declaration
   public function Open()
   {
      switch($this->type)
      {
         case "xhtml10s":
            //header('content-type: application/xhtml+xml');
            echo '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">' . NEW_LINE;
            echo '<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">' . NEW_LINE;
            break;

         case "xhtml10t":
            //		header("content-type: application/xhtml+xml");
            echo '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">' . NEW_LINE;
            echo '<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">' . NEW_LINE;
            break;

         case "xhtml11":
            //		header("content-type: application/xhtml+xml");
            echo '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.1//EN" "http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd">' . NEW_LINE;
            echo '<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">' . NEW_LINE;
            break;

         default:
            echo "UNKNOWN/UNSOPORTED HTML CODING";
      }
      $this->_isOpen = true;
   }

   public function OpenBody()
   {
      $ret = "";
      if( ! $this->_isOpen)
      {
         $ret .= $this->Open();
      }
      if( ! $this->_isHeaderSent
      )
      {
         $ret .= $this->RenderHead();
      }
      $ret .= NEW_LINE;
      $ret .= HtmlElement("body", ""/*$HtmlBody*/, NULL, HTML_ELEMENT_OPEN);
      $ret .= NEW_LINE;
      return $ret;
   }

   public function RenderHead()
   {
      $ret = "";
      if( ! $this->_isOpen)
      {
         $this->Open();
      }
      $head = "";
      $head .= $this->RenderMeta();
      $head .= $this->RenderTitle();
      $head .= $this->RenderStyle();
      $head .= $this->RenderLink();
      $head .= $this->RenderScript();
      $this->_isHeaderSent = true;
      return $ret . HtmlElement("head", NULL, $head . NEW_LINE);
   }

   // header-css

   /**
    Adds style file to link or style depending configuration.
    Media:
    *screen     Intended for non-paged computer screens.
    tty         Intended for media using a fixed-pitch character grid, such as teletypes, terminals, or portable devices with limited display capabilities.
    tv          Intended for television-type devices (low resolution, color, limited scrollability).
    projection  Intended for projectors.
    handheld    Intended for handheld devices (small screen, monochrome, bitmapped graphics, limited bandwidth).
    print       Intended for paged, opaque material and for documents viewed on screen in print preview mode.
    braille     Intended for braille tactile feedback devices.
    aural       Intended for speech synthesizers.
    all         Suitable for all devices.
    */
   public function AddCssFile($filename, $media)
   {
      if($this->cssAsStyle)
      {
         // CSS out 2: style
         // <style type="text/css" media="all">@import "/files/css/82bccec9b4c6e2e05915262ef6984c63.css";</style>
         // <style type="text/css" media="print">@import "/themes/tsvn_garland/print.css";</style>
         // can import other then screen media
         $this->_AddCssStyle("@import \"$filename\";", $media);
      }
      else
      {
         // CSS out 1: link
         // <link rel="stylesheet" href="styles_html.css" type="text/css">
         // <link href="images/main.css" rel="stylesheet" type="text/css">
         // <link href="/_javascript/jLook/templates/default/default.css" rel="stylesheet" type="text/css" />
         $link = array(
                       "href"=>$filename,
                       "rel"=>"stylesheet",
                       "type"=>
                       HTML_STYLE_CSS);
         if(isset($media))
         {
            $link["media"] = $media;
         }
         $this->AddLink($link);
      }
   }

   /**
    Adds Css style included
    */
   public function AddCssStyle($style, $media)
   {
      $newStyle = array(
                        "type"=>HTML_STYLE_CSS,
                        "media"=>$media,
                        "/"=>$style
                        );
      $this->_AddStyle($newStyle);
   }

   // header:link
   // <link href="styles_html.css" rel="stylesheet" type="text/css">
   // <link href="images/main.css" rel="stylesheet" type="text/css">
   // <link href="default.css" rel="stylesheet" type="text/css" />
   // <link href="" rel="alternate" type="application/rss+xml" title=""  />
   // <link href="favico.ico" rel="shortcut icon" type="image/x-icon"  />

   public function AddLink($a, $b = NULL)
   {
      // FIX ME
      if($b == NULL && is_string($a))
      {
         $filename = $a;
      }
      elseif(is_array($a) && $b == NULL)
      {
         $link = $a;
      }
      elseif(is_string($a) && is_array($b))
      {
      }
      $this->_header["link"][] = $link;
   }

   public function RenderLink()
   {
      $ret = "";
      $links = $this->_header["link"];
      if(isset($links) && count($links) > 0)
      {
         $ret .= NEW_LINE;
         $ret .= NEW_LINE_TAB . "<!-- LINK -->";
         foreach($links as $link)
         {
            if( ! is_array($link))
            {
               $link = array("href"=>$link);
            }
            if(count($link) > 0)
            {
               $ret .= NEW_LINE_TAB . HtmlElement("link", $link, NULL, HTML_ELEMENT_SMARTCLOSE);
            }
         }
      }

      return $ret;
   }

   // header:meta
   public function AddMeta($name, $content)
   {
      $meta = array(
                    "name"=>$name,
                    "content"=>$content
                    );
      $this->_header["meta"][] = $meta;
   }

   public function AddMetaHttpEquiv($name, $content)
   {
      $meta = array(
                    "http-equiv"=>$name,
                    "content"=>$content
                    );
      $this->_header["meta"][] = $meta;
   }

   public function RenderMeta()
   {
      $ret = "";
      $metas = $this->_header["meta"];
      if(isset($metas) && count($metas) > 0)
      {
         $ret .= NEW_LINE;
         $ret .= NEW_LINE_TAB . "<!-- META -->";
         foreach($metas as $meta)
         {
            if(is_array($meta) && count($metas) > 0)
            {
               $ret .= NEW_LINE_TAB . HtmlElement("meta", $meta, NULL, HTML_ELEMENT_SMARTCLOSE);
            }
         }
      }

      return $ret;
   }

   // header:style
   // <style type="text/css" media="all">@import "/files/css/82bccec9b4c6e2e05915262ef6984c63.css";</style>
   // <style type="text/css" media="print">@import "/themes/tsvn_garland/print.css";</style>

   public function AddStyle($style, $media = NULL, $type = HTML_STYLE_CSS)
   {
      $neStyle = $style;
      if(is_string($style))
      {
         $newStyle = array();
         $newStyle["/"] = $style;
      }
      if(isset($type))
      {
         $newStyle["type"] = $type;
      }
      if(isset($media))
      {
         $newStyle["media"] = $media;
      }
      else
         if($b == NULL && is_string($a))
         {
            $filename = $a;
         }
         elseif(is_array($a) && $b = NULL)
         {
         }
         elseif(is_string($a) && is_array($b))
         {
         }
         //$type=
      $this->_header["style"][] = $newStyle;
   }

   public function RenderStyle()
   {
      $ret = "";
      $styles = $this->_header["style"];
      if(isset($styles))
      {
         $ret .= NEW_LINE;
         $ret .= NEW_LINE_TAB . "<!-- STYLE -->";
         $styleArgs = array("rel"=>"stylesheet", "type"=>HTML_STYLE_CSS);
         foreach($htmlStyles as $style)
         {
            if(is_array($style))
            {
               $args = array_merge($styleArgs, $style);
            }
            else
            {
               $args = array_merge($styleArgs, array("href"=>$style));
            }
            $ret .= NEW_LINE_TAB . HtmlElement("style", $args, NULL, HTML_ELEMENT_SMARTCLOSE);
         }
      }

      return $ret;
   }

   // header:script
   // <script type="text/javascript" src="script.js"></script>

   public function AddScript($src = NULL, $body = NULL, $type = NULL)
   {
      $newScript = array();
      if(is_string($src))
      {
         $newScript["src"] = $src;
      }
      if(is_string($body))
      {
         $newScript["/"] = $type;
      }
      if(is_string($type))
      {
         $newScript["type"] = $type;
      }
      if(count($newScript))
      {
         $this->_header["script"][] = $newScript;
      }
   }

   public function RenderScript()
   {
      $ret = "";
      $scripts = $this->_header["script"];
      if(isset($scripts))
      {
         $ret .= NEW_LINE;
         $ret .= NEW_LINE_TAB . "<!-- SCRIPT -->";
         $scriptDefaultArgs = array("type"=>HTML_SCRIPT_JAVASCRIPT);
         foreach($scripts as $script)
         {
            $scriptArgs = $scriptDefaultArgs;
            if(is_array($script))
            {
               $args = array_merge($scriptArgs, $script);
            }
            else
            {
               $args = array_merge($scriptArgs, array("src"=>$script));
            }
            $ret .= NEW_LINE_TAB . HtmlElement("script", $args, NULL, HTML_ELEMENT_CLOSE);
         }
      }

      return $ret;
   }

   // header:title
   public function SetTitle($title)
   {
      $this->_header["title"] = $title;
   }
   public function RenderTitle()
   {
      $title = $this->_header["title"];
      $ret = "";
      if(isset($title))
      {
         $ret .= NEW_LINE;
         $ret .= NEW_LINE_TAB . "<!-- TITLE -->";
         $ret .= NEW_LINE_TAB . HtmlElement("title", NULL, $title);
      }
      return $ret;
   }
}

function HtmlElement($Element, $Args = NULL, $text = NULL, $closed = HTML_ELEMENT_SMARTCLOSE)
{
   $ret = "";
   $ret .= "<$Element";
   if(is_array($Args))
   {
      ksort($Args);
      foreach($Args as $key=>$value)
      {
         if($key != "/")
         {
            $ret .= " $key=\"$value\"";
         }
      }
   }
   if($closed == HTML_ELEMENT_SMARTCLOSE)
   {
      if("$text" != "")
      {
         $closed = HTML_ELEMENT_CLOSE;
      }
      else
      {
         $closed = HTML_ELEMENT_SELFCLOSE;
      }
   }
   switch($closed)
   {
      case HTML_ELEMENT_OPEN:
         $ret .= ">";
         break;

      case HTML_ELEMENT_SELFCLOSE:
         $ret .= " />";
         break;

      case HTML_ELEMENT_CLOSE:
         $ret .= ">$text</$Element>";
         break;
   }
   return $ret;
}
//----------------------------------------------------------------------------


function HtmlHeaderElement($HtmlType, $HtmlHeaderElementType, $HtmlHeaderElement, $HtmlElementClose = HTML_ELEMENT_SMARTCLOSE)
{
   if(count($HtmlHeaderElement) > 1 || is_array($HtmlHeaderElement) ||  ! isset($HtmlHeaderElement["/"]))
   {
      return NEW_LINE_TAB . HtmlElement($HtmlHeaderElementType, $HtmlHeaderElement, $HtmlHeaderElement["/"], $HtmlElementClose);
   }
}
//----------------------------------------------------------------------------


function HtmlHeaderElements($HtmlType, $HtmlHeaderElementType, $HtmlHeaderElements, $HtmlElementClose = HTML_ELEMENT_SMARTCLOSE)
{
   if( ! is_array($HtmlHeaderElements))
   {
      return;
   }
   $ret = "";
   foreach($HtmlHeaderElements as $HtmlHeaderElement)
   {
      $ret .= HtmlHeaderElement($HtmlType, $HtmlHeaderElementType, $HtmlHeaderElement, $HtmlElementClose);
   }
   return $ret;
}
//----------------------------------------------------------------------------


function HtmlHeader($HtmlType, $HtmlHeader = NULL, $HtmlBody = NULL)
{
   switch($HtmlType)
   {
      case "xhtml10s":
         //header('content-type: application/xhtml+xml');
         echo '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">' . NEW_LINE;
         echo '<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">' . NEW_LINE;
         break;

      case "xhtml10t":
         //		header("content-type: application/xhtml+xml");
         echo '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">' . NEW_LINE;
         echo '<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">' . NEW_LINE;
         break;

      case "xhtml11":
         //		header("content-type: application/xhtml+xml");
         echo '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.1//EN" "http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd">' . NEW_LINE;
         echo '<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">' . NEW_LINE;
         break;

      default:
         echo "UNKNOWN/UNSOPORTED HTML CODING";
   }

   echo '<head>';
   echo NEW_LINE_TAB . "<!-- CONTENT INFO -->";
   echo NEW_LINE_TAB . "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />";
   echo NEW_LINE_TAB . HtmlElement("meta", array("http-equiv"=>"Content-Type", "content"=>"text/html; charset=UTF-8"));
   echo NEW_LINE_TAB . HtmlElement("meta", array("name"=>"keywords", "content"=>$HtmlHeader["keywords"]));
   echo NEW_LINE_TAB . HtmlElement("meta", array("name"=>"description", "content"=>$HtmlHeader["description"]));
   echo NEW_LINE;
   echo NEW_LINE_TAB . "<!-- TITLE -->";
   echo NEW_LINE_TAB . HtmlElement("title", NULL, $HtmlHeader["title"]);
   echo NEW_LINE;
   echo NEW_LINE_TAB . "<!-- FAVICON -->";
   //	echo NEW_LINE_TAB."<link rel=\"shortcut icon\" href=\"favicon.ico\" />";
   echo NEW_LINE_TAB . HtmlElement("link", array("rel"=>"shortcut icon", "href"=>"favicon.ico"));
   echo NEW_LINE;

   /*	if (isset($HtmlHeader["style"])) {
   		echo NEW_LINE_TAB."<!-- STYLE -->";
   		HtmlHeaderElements($HtmlType, "style", $HtmlHeader["style"]);
   	}//*/

   $htmlStyles = $HtmlHeader["style"];
   if(isset($htmlStyles))
   {
      echo NEW_LINE_TAB . "<!-- STYLE -->";
      $styleArgs = array("rel"=>"stylesheet", "type"=>HTML_STYLE_CSS);
      foreach($htmlStyles as $style)
      {
         if(is_array($style))
         {
            $args = array_merge($styleArgs, $style);
         }
         else
         {
            $args = array_merge($styleArgs, array("href"=>$style));
         }
         echo NEW_LINE_TAB . HtmlElement("link", $args, NULL, HTML_ELEMENT_SMARTCLOSE);
      }
   }

   $htmlScripts = $HtmlHeader["script"];
   /*	if (isset($htmlScripts)) {
   		echo NEW_LINE;
   		echo NEW_LINE_TAB."<!-- SCRIPT -->";
   		$styleArgs=array("type" => HTML_STYLE_CSS);
   		foreach ($htmlStyles as $style) {
   			if (is_array($style)) {
   				$args=array_merge($styleArgs, $style);
   			} else {
   				$args=array_merge($styleArgs, array("href" => $style));
   			}
   			echo NEW_LINE_TAB.HtmlElement("script", $args, NULL, HTML_ELEMENT_SMARTCLOSE);
   		}
   	}//*/
   if(isset($htmlScripts))
   {
      echo NEW_LINE;
      echo NEW_LINE_TAB . "<!-- SCRIPT -->";
      HtmlHeaderElements($HtmlType, "script", $htmlScripts, HTML_ELEMENT_CLOSE);
   }
   /*	if (isset($css)) {
   		echo "<link rel=\"stylesheet\" href=\"$css\" type=\"text/css\" />";
   	}//*/
   echo NEW_LINE;
   echo '</head>';
   echo NEW_LINE;
   echo NEW_LINE;
   echo HtmlElement("body", $HtmlBody, NULL, HTML_ELEMENT_OPEN);
   echo NEW_LINE;
}
//----------------------------------------------------------------------------


function HtmlFooter($HtmlType)
{
   echo "</body>" . NEW_LINE . "</html>";
}
//----------------------------------------------------------------------------
