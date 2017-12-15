<?php

namespace Drupal\page_archive;

use HTMLPurifier;
use HTMLPurifier_Config;

/**
 * Class ContentScraper
 * @package Drupal\page_archive
 */
class ArchiveContentScraper
{
  // user input variables
  private $url;
  private $tags = [];
  // main content storage
  private $content = '';
  private $metaTags = [];
  // class variables
  private $rawContent;
  private $dom = FALSE;
  // error flag
  private $error = FALSE;
  private $pullImages;

  /**
   * ArchiveContentScraper constructor.
   * @param string $url
   * @param array $tags
   * @param $pullImages
   */
  public function __construct($url = '', $tags = [], $pullImages = TRUE) {
    $this->setUrl($url); // set user url
    $this->setTags($tags); // set usr tags
    $this->pullImages = $pullImages; // set images
    //  call curl to get the html markup string from the url user provided.
    $this->rawContent = $this->curl();
    if (!$this->isError()) {
      // initiate the dom object
      $this->dom = new \PHPHtmlParser\Dom();
      // load the html into the dom object.
      $this->dom->loadStr($this->rawContent, []);
    }

  }

  /**
   * Build content method will get the set tags and scraper the content between the tags.
   * It will then grab images based on the pullImages flag. True get images false delete img tags.
   * Remove inline style from any where in the content.
   */
  public function buildContent() {
    // loop through all the tags and find use dom find method to locate the elements.
    if (sizeof($this->getTags()) > 0) {
      foreach ($this->getTags() as $tag) {
        $contents = $this->dom->find($tag);
        // Loop through the contents of the the find method and content return.
        if (sizeof($contents) > 0) {
          foreach ($contents as $content) {
            $combine_content = $this->getContent() . $content->outerHtml();
            // Join all relevant content together.
            $this->setContent($combine_content);
          }
        } else {
          // If there no content then report error
          watchdog(
            'Page Archive',
            "Error in loading content from parser.",
            NULL,
            WATCHDOG_ERROR
          );
          $this->setError(TRUE);
        }
      }
    } else {
      // if no css selectors where passed.
      watchdog(
        'Page Archive',
        "Need valid css selector",
        NULL,
        WATCHDOG_ERROR
      );
      $this->setError(TRUE);
    }

    if ($this->pullImages) {
      $this->setContent($this->buildImage());
    } else {
      // remove image tags
      $this->setContent($this->removeImgTags());
    }
    // remove inline styles
    $this->setContent($this->removeInlineStyles());
  }

  /**
   * Will remove all image tags from the content
   * @return mixed
   */
  public function removeImgTags() {
    $dom = new \PHPHtmlParser\Dom(); // new dom object to hold just content
    $dom->loadStr($this->getContent(), []); // get content loaded into the dom object
    $imgs = $dom->find('img'); // find img tags
    // loop through images and remove them from content dom
    foreach ($imgs as $img) {
      $img->delete();
    }
    unset($imgs); // send images to garbage collection
    $html = $dom->outerHtml;
    unset($dom); // send dom to garbage collection
    return $html; // return current html string from dom.
  }


  /**
   * Will pull all image files and create a space in files/archive for them to be saved.
   * will add the new path to the images.
   *
   * @return mixed
   */
  public function buildImage() {
    $dom = new \PHPHtmlParser\Dom();
    $dom->loadStr($this->getContent(), []);
    $img_dom = $dom->find('img');

    for ($i = 0; $i < sizeof($img_dom); $i++) {
      $img = $dom->find('img')[$i];
      $img_src = $img->getAttribute('src');

      if (substr($img_src, 0, 4) !== 'http') {
        $parse = parse_url($this->getUrl());

        if ($img_src[0] === '/') {
          $img_url = $parse['scheme'] . '://' . $parse['host'] . $img_src;
          $img_file = file_save_data(
            file_get_contents($img_url),
            file_default_scheme() . '://archive/image' . basename($img_src),
            FILE_EXISTS_RENAME
          );

        } else {
          $img_file = file_save_data(
            file_get_contents($parse['scheme'] . '://' . $parse['host'] . '/' . $img_src),
            file_default_scheme() . '://archive/image' . basename($img_src),
            FILE_EXISTS_RENAME
          );
        }
        $new_img_url = parse_url(file_create_url($img_file->uri));
        $img->setAttribute('src', $new_img_url['path']);
      }
    }

    // Clean up memory
    unset($img_dom);
    $html = $dom->outerHtml;
    unset($dom);

    return $html;
  }

  /**
   * Remove all inline style tags
   * @return mixed
   */
  public function removeInlineStyles() {
    $html = preg_replace('/style=\\"[^\\"]*\\"/', '', $this->getContent());
    return $html;
  }

  /**
   * First fetch in the grabbing all the content on a given url
   * Purify the output string and store the clean results in the dom variable.
   * @return string
   */
  public function curl() {
    // Assigning cURL options to an array
    $url = $this->getUrl();

    $data = file_get_contents($url); //Get content
    if ($data == FALSE) {
      watchdog(
        'Page Archive',
        "Error occurred when $url was cURL",
        NULL,
        WATCHDOG_ERROR
      );
      $this->setError(TRUE);
    } else {
      // Using the purifier library to clean that data object
      $this->buildMetaTags();
      $cleanerConfig = HTMLPurifier_Config::createDefault();
      $cleaner = new HTMLPurifier($cleanerConfig);
      $formattedHTML = $cleaner->purify($data);
      unset($cleanerConfig); // get ready for garbage collection
      unset($cleaner); // get ready for garbage collection
      return $formattedHTML;
    }

    return NULL;
  }

  /**
   * This will get the metatags form the url and put them in an array then store them in memory.
   */
  public function buildMetaTags() {
    // Get meta tag information from url head.
    $meta_tags = get_meta_tags($this->getUrl());
    // tag container
    $built_tags = [];
    // Loop through the meta tags and build render array
    foreach ($meta_tags as $key => $tag) {
      $element = [
        LANGUAGE_NONE => [
          $key => [
            'value' => $tag
          ]
        ]
      ];
      // add item onto the tags array
      $built_tags[] = $element;
    }
    // set the meta tags.
    $this->setMetatags($built_tags);
  }

  /**
   * @return array
   */
  public function getmetaTags() {
    return $this->metaTags;
  }

  /**
   * @param array $metaTags
   */
  public function setmetaTags($metaTags) {
    $this->metaTags = $metaTags;
  }

  /**
   * @return array
   */
  public function getTags() {
    return $this->tags;
  }

  /**
   * @param array $tags
   */
  public function setTags($tags) {
    $this->tags = $tags;
  }

  /**
   * @return mixed
   */
  public function getContent() {
    return $this->content;
  }

  /**
   * @param mixed $content
   */
  public function setContent($content) {
    $this->content = $content;
  }

  /**
   * @return mixed
   */
  public function getUrl() {
    return $this->url;
  }

  /**
   * @param mixed $url
   */
  public function setUrl($url) {
    $this->url = $url;
  }

  /**
   * @return bool
   */
  public function isError() {
    return $this->error;
  }

  /**
   * @param bool $error
   */
  public function setError($error) {
    $this->error = $error;
  }

  /**
   * @return string
   */
  public function getRawContent() {
    return $this->rawContent;
  }

  /**
   * @param string $rawContent
   *
   * @return ArchiveContentScraper
   */
  public function setRawContent($rawContent) {
    $this->rawContent = $rawContent;
    return $this;
  }

  /**
   * @return bool|\PHPHtmlParser\Dom
   */
  public function getDom() {
    return $this->dom;
  }

  /**
   * @param bool|\PHPHtmlParser\Dom $dom
   *
   * @return ArchiveContentScraper
   */
  public function setDom($dom) {
    $this->dom = $dom;
    return $this;
  }

}
