<?php

/**
 * @file
 * Provides the contolling logic of the si_publications module
 */

/**
 * Loads scientist nodes that have their SI Profile ID field != NULL and returns them as an array of node objects
 *  
 */
function si_publications_load_scientists() {
  $scientist_ct = variable_get('si_publications_scientist_ct');
  $si_profile_field = variable_get('si_publications_si_profile_field');
  
  if ($scientist_ct) {
    $query = new EntityFieldQuery();
    $query->entityCondition('entity_type', 'node')
          ->entityCondition('bundle', $scientist_ct)
          //only pull scientists that have their si profile field set
          ->fieldCondition($si_profile_field, 'value', 'NULL', '!=')
          //limit the number of staff nodes queried.
           // ->range(0, 5)
          ;
    $result = $query->execute();
    $nids = array_keys($result['node']);
    $staff = node_load_multiple($nids);
    return $staff;
  }   
  else
  {
    drupal_set_message('You must choose a scientist content type on the settings page', 'error');
    return null;
  }
}

/**
 * Retrieves JSON from SI libraries and saves it to SCBI staff nodes
 *  
 */
function si_publications_fetch($node) {

  //create static variables for efficiency
  $si_profile_field = &drupal_static('si_profile_field');
  if (!isset($si_profile_field)) {
    $si_profile_field = variable_get('si_publications_si_profile_field');
  }

  $recent_papers_field = &drupal_static('recent_papers_field');
  if (!isset($recent_papers_field)) {
    $recent_papers_field = variable_get('si_publications_recent_papers_field');
  }

  $paper_quantity = &drupal_static('paper_quantity');
  if (!isset($paper_quantity)) {
    $paper_quantity = variable_get('si_publications_papers_quantity', 3);
  }

  $author_id = $node->{$si_profile_field}['und'][0]['value'];

  //set up SIL API call
  $url = 'http://prism.si.edu/sil/researchbib/srb_search_export_action.cfm';
  $data = array(
    'search_term' =>  $author_id,
    // 'submit' => 'Export+data',
    'date' => '',
    'format' => 'JSON',
    'unit' => 'All',
    'limit' => 'authorID',
    // 'count' => '5'
  );

  $full_url = url($url, array('query' => $data));
  // dpm($full_url);

  //Make call and parse results
  $result = drupal_http_request($full_url);

  if ($result) {
    $pubs = drupal_json_decode($result->data);
  }

  try {
    if ($pubs) {
      //discard all but the first x results where x is the number of recent papers to store set by the user in settings
      $recent_pubs = array_slice($pubs, 0, $paper_quantity);

      // we are starting the import for a new scientist, so set $n, which is the paper counter, to 0
      // and set $updated, which tracks whether we will need to update their node, to FALSE
      $n = 0;
      $updated = FALSE;

      //parse each paper
      //stop at the paper quantity limit set by the user OR when the number of papers found for the scientist is reached
      while ($n < $paper_quantity && $n < count($recent_pubs)) {
        foreach($recent_pubs as $pub => $reference) {
          foreach($reference as $pub) {
            foreach($pub as $unformatted_pub) {
              //transform the array into a formatted string
              $formatted_pub = '<p class="paper-citation">' . si_publications_format($unformatted_pub) . '</p>';
              //add this paper to node if one of the following is true: (1) it is the first paper being added to an empty field (2) it is an additional paper ( ie no paper exists at this spot yet), or (3) if it is different from exisiting paper in this spot
              if ( empty($node->{$recent_papers_field}) || !array_key_exists($n, $node->{$recent_papers_field}[LANGUAGE_NONE]) || $formatted_pub != $node->{$recent_papers_field}[LANGUAGE_NONE][$n]['value'] ) {

                $node->{$recent_papers_field}[LANGUAGE_NONE][$n] = array(
                  'value' => t($formatted_pub),
                  'format' => 'full_html',
                  );

                //a paper was changed so we will need to save the node
                $updated = TRUE;
              }
            $n++;
            }
          }
        }
      }
      // clear any remaining publications above the paper quanity limit that exist in the db
      while ( array_key_exists($n, $node->{$recent_papers_field}[LANGUAGE_NONE]) ) {
        unset($node->{$recent_papers_field}[LANGUAGE_NONE][$n]);
        $n++;
        $updated = TRUE;
      }
      //if any papers were changed, save the node
      if ($updated) {
        node_save($node);
        $message = 'Successfully imported publications for ' . l($node->title, '/node/' . $node->nid, array('attributes' => array('target' => '_blank')));
        // drupal_set_message($message);
        watchdog('si_publications', $message);
        return $message;
      }
    }
    else {
      throw new Exception('No results found for that SI profile ID');
    }
  }
  catch (exception $e) {
    $message = 'Tried importing publications for ' . $node->title . ' (using SI Profile ID ' . $author_id . ') and failed. ' . $e->getMessage();
    // drupal_set_message($message, 'error');
    watchdog('si_publications', $message);
    return $message;
  }
}

/**
 * Input: an array containing pieces of a publication
 * Returns: A string of a beautifully formatted publication 
 */
function si_publications_format($unformatted_pub) {
  $authors    = $unformatted_pub['author_display'];
  //if $authors did not end with a period, add one because there needs to be a period between $authors and $date
  $authors    = substr($authors, -1) != '.' ? ($authors . '.') : $authors;
  $date       = $unformatted_pub['date'];
  $title      = $unformatted_pub['title'];
  //if a repository link exists, wrap the title in it
  $title      = isset($unformatted_pub['link']) ? ( '<a href="' . $unformatted_pub['link'] . '">' . $title . '</a>' ) : $title;
  $doi        = isset($unformatted_pub['doi']) ?   (' <a href="' . $unformatted_pub['doi'] . '">' . $unformatted_pub['doi'] . '</a>' ) : null;
  $pages      = isset($unformatted_pub['pages']) ? (', ' . $unformatted_pub['pages']) : null;
  $journal    = isset($unformatted_pub['journal']) ? ( '<em>' . $unformatted_pub['journal'] . '</em>' ) : null;

  //put the paper reference in our format
  $formatted_pub = $authors . ' ' . $date . '. ' . '<strong>' . $title . '</strong>. ' . $journal . $pages . '.' . $doi;
  return $formatted_pub; 
}

/**
 * Dynamically generates a list of all papers for the scientist and embeds on a page (does not store in db)
 */
function si_publications_get_all_papers($node) {
  // //get system path for this scientist (nzp specific)
  // $alias = 'conservation/' . $scientist;
  // if ( $path = drupal_lookup_path("source", $alias) ) {
  //     $node = menu_get_object("node", 1, $path);
  // }
  // else {
  //   // drupal_not_found();
  //   // drupal_exit();
  // }

  // fetch the scientist content type and SI profile ID field from this module's settings
  $scientist_ct = variable_get('si_publications_scientist_ct');
  $si_profile_field = variable_get('si_publications_si_profile_field');

  //check that this node we just loaded really is a scientist and that she has an SI profile ID
  if ( !($node->type == $scientist_ct && !empty($node->{$si_profile_field})) ) {
    drupal_not_found();
    drupal_exit();
  }

  //set breadcrumb (specific to NZP)
  // $crumbs = array();
  // $crumbs[] = l(t('Home'), '');
  // $crumbs[] = l(t('Conservation'), 'conservation');
  // $crumbs[] = l(t('About SCBI'), 'conservation/about-scbi');
  // $crumbs[] = l(t('SCBI Staff'), 'conservation/scbi-staff');
  // $crumbs[] = l($node->title, 'conservation/' . $scientist);
  // drupal_set_breadcrumb($crumbs);

  //grab the si profiles ID and get the papers from SIL
  $si_profiles_id = $node->{$si_profile_field}['und'][0]['value'];
  $url = 'http://prism.si.edu/sil/researchbib/srb_search_export_action.cfm?search_term=' . $si_profiles_id . '&submit=Export+data&date=&limit=authorID&format=JSON&Unit=All&count=2500';

  $result = drupal_http_request($url);

  if ($result) {
    $pubs = drupal_json_decode($result->data);
  }
  //if no papers found, return 404
  else {
    drupal_not_found();
    drupal_exit();
  }
  
  // Iterate through papers and format them
  $all_papers = '';

  //add altmetric functionality if enabled.
  $altmetric_set = add_altmetric();

  foreach($pubs as $pub => $reference) {
    foreach($reference as $pub) {
      foreach($pub as $unformatted_pub) {
        //transform the array into a formatted string
        $formatted_pub = si_publications_format($unformatted_pub);
        $altmetric  = ($altmetric_set && isset($unformatted_pub['doi'])) ? '<div id="show_metric" class="altmetric-embed altmetricIcon" data-badge-type="donut" data-badge-popover="left" data-hide-no-mentions="true" data-doi="' . $doi_stripped = str_replace('http://dx.doi.org/', '', $unformatted_pub['doi']) . '"></div>' : null;
        $all_papers .= '<p class="paper-citation">' . $altmetric . $formatted_pub . '</p>';
      }
    }
  }
  return $all_papers;
}

function add_altmetric() {
  $altmetric_set = &drupal_static('altmetric_set');
  if (!isset($altmetric_set)) {
    $altmetric_set = variable_get('si_publications_altmetric_switch');
  }

  if ($altmetric_set) {
     //add altmetric external js file
    drupal_add_js('https://d1bxh8uas1mnw7.cloudfront.net/assets/embed.js', array(
      'scope' => 'header',
      'type' => 'external',
      'every_page' => FALSE,
    ));

    drupal_add_css(
      '
      .altmetricIcon {
        float:right;
      }

      .altmetricIcon img {
        float: right;
        margin-left: 25px !important;
      }
      '
      , 'inline'
    );
    return TRUE;
  }
}
