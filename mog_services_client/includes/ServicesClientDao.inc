<?php

/**
 * @file Operations with services_client
 */

/**
 * ServicesClientDao contains api methods.
 */
class ServicesClientDao {

  protected $resource = NULL;

  protected $api = NULL;

  /**
   * Creates instance of api.
   */
  function __construct($resource) {
    $this->resource = $resource;
    try {
      $this->api = services_client_connection_get($this->resource);
    }
    catch (ServicesClientConnectionResponseException $e) {
      $this->handleException($e);
    }
  }

  /**
   * Gets variable from remote drupal server.
   *
   * @param unknown $variable_name
   * @return unknown
   */
  function getVariable($variable_name) {
    $variable = NULL;
    try {
      $api_result = $this->api->action('system', 'get_variable', array('name' => $variable_name));
      $variable = $api_result;
    }
    catch (ServicesClientConnectionResponseException $e) {
      $this->handleException($e);
    }
    return $variable;
  }

   /**
   * Gets node from remote drupal server.
   *
   * @param $nid
    *  Node id.
   * @return
    * Node object
   */
  function getNode($nid) {
    $node = NULL;
    try {
      $api_result = $this->api->get('node', $nid);
      $node = $api_result;
    }
    catch (ServicesClientConnectionResponseException $e) {
      $this->handleException($e);
    }
    return $node;
  }

   /**
   * Gets nodes from remote drupal server.
   *
   */
  function getNodes() {
    $nodes = array();
    try {
      $api_result = $this->api->get('node');
      $nodes = $api_result;
    }
    catch (ServicesClientConnectionResponseException $e) {
      $this->handleException($e);
    }
    return $nodes;
  }

  /**
   * Function for sending of node to remote drupal server.
   *
   * @param $node
   *   Node object to send.
   * @param  $filefield_names
   *   Array of names of fields which contain files. Following files will be sent
   *   to server separately.
   * @return
   *   NULL on error, nid from remote drupal on success.
   */
  function sendNode($node, $filefield_names) {
    $result = NULL;
    try {
      // Unset conflicting parts, which could create constraint errors on remote server.
      unset($node->nid);
      unset($node->vid);
      unset($node->path);
      unset($node->uid); // => we want to use uid of server instead of client.
      unset($node->name); // => if name is set there will be used uid from client.

      // For each file field name load file values and sent it to remote server,
      foreach ($filefield_names as $file_field) {
        if (!empty($node->{$file_field}[LANGUAGE_NONE])) {
          // Iterate over all files in field.
          foreach ($node->{$file_field}[LANGUAGE_NONE] as $key => $file) {
            // Load file by fid  to ensure, we have proper structure.
            $file = (array) file_load($file['fid']);
            $file_raw = array(
              'filesize' => filesize($file['uri']),
              'filename' => $file['filename'],
              'file' => base64_encode(file_get_contents($file['uri'])),
            );
            // Sent file to remote server.
            $api_result = $this->api->create('file', $file_raw);
            if (isset($api_result['fid'])) {
              // Update fid reference to remote server fid.
              $node->{$file_field}[LANGUAGE_NONE][$key]['fid'] = $api_result['fid'];
            }
          }
        }
      }
      // Sent node to remote server, with uptaded fids from remote server.
      $api_result = $this->api->create('node_raw', (array) $node);
      if (isset($api_result['nid'])) {
        // Set remote server nid as result.
        $result = $api_result['nid'];
      }
    }
    catch (ServicesClientConnectionResponseException $e) {
      $this->handleException($e);
    }
    return $result;
  }

  /**
   * Gets list of terms from vocabulary from remote drupal server.
   *
   * @param $vid
   *   Vocabulary id.
   * @return
   *   Array of terms in format array('tid' => 'name').
   */
  function getVocabularyTerms($vid) {
    $list = array();
    try {
      $api_result = $this->api->action('taxonomy_vocabulary', 'getTree', array('vid' => $vid));
      foreach ($api_result as $result) {
        $list[$result['tid']] = $result['name'];
      }
    }
    catch (ServicesClientConnectionResponseException $e) {
      $this->handleException($e);
    }
    return $list;
  }

  /**
   * Debug functionality, which returns last response.
   *
   * @see ServicesClientConnection->getDebug();
   */
  function getDebug() {
    return $this->api->getDebug();
  }

  /**
   * General exception handling.
   *
   * @param $exception
   *   Exception to handle.
   */
  function handleException($exception) {
    // Log exception on error.
    $exception->log();
    // Set error message.
    drupal_set_message(t('Connecting to remote server failed. Please try to reload the page. If problems persist please contact site administrator.'), 'error');
  }
}
