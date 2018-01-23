SI publications
====================================

This module was written by Anthony Braun at NZP. If you have any questions or want to help improve this module, please email me at brauna@si.edu

### SUMMARY
 The SI Publications module allows you to import publications for your unit's scientists from Smithsonian Libraries.
	This can be done manually or automatically as a cron task using the Drupal queue API.

    
### REQUIREMENTS
 You must have created a "scientist" content type (it can be named anything you like). This content type MUST contain the following two
	fields:
 1. A text field for the scientist's SI Profiles ID (available here: https://profiles.si.edu/). This field can be named anything,
	but it must be a text field.
 2. A longtext field for recent papers. This must allow unlimited values and it must be HTML-enabled. Again, the field name does not
	matter.
 3. Each new content type comes with a default title field. You can choose to hide it or use it as you like.

### OTHER PREPARATION
 For each scientist or scholar for whom you want to import publications data into your website, create a record or node of the "scientist"
	content type (or whatever you called your new content type):
 1. If you use the title field, one way to use it is to fill in the name of the scientist.
 2. In order to get the scientist's SI Profiles ID, you need to search for the scientist at https://profiles.si.edu/search by name.
	Once you find a scientist's profile, the SI Profiles ID will be part of the URL. For example, a search for Torsten Dikow will result
	in a 	profile record at https://profiles.si.edu/display/nDikowT962012. The profile ID is DikowT962012.
 3. Leave your recent papers field blank. This field will be automatically populated when the import is done.


### INSTALLATION
 Install as usual, see http://drupal.org/documentation/install/modules-themes/modules-7 for further information

### CONFIGURATION
 Go to /admin/content/publications/settings and fill out the form.

 1. Your Scientist Content Type: Select your "scientist" content type from the list of all your content types.
 2. Your SI profile ID field: Select your "SI profile ID" field from the listing of fields for the scientist content type.
 3. Your Recent Papers field: Select your "Recent Papers" field from the listing of fields for the scientist content type.
 4. Check the "Import via cron" box in order to get an up-to-date import every time a cron job runs.
 5. Number of papers to store for each scientist: The default is three (3), the maximum is ten (10).
 6. Include Altmetric icons next to citations: Check this box if you want an Altmetric badge to appear next to each publication that has a
	valid DOI.
 7. Once you save your publication import settings, you can go to the Publications Import tab at /admin/content/publications in order to
	run a manual import the first time.
 8. For each scientist record/node you should now see the three to ten recent papers imported into it. At the bottom of the record/node
	you should also see an "All Papers" link to an all papers node for that scientist.


If you checked the "Import via cron" box on the Publications Settings form above, you should get updated imports every time a cron job
runs. If you did not check this box, you will need to run updated imports manually from the Publications Import tab.

### MODIFICATIONS MADE BY JIM KOCHERT, NMNH WEBSITE ADMINISTRATOR
JAK Modifications to si_publications.controller.inc
=====================================================

~ lines 62-63 and 201-202: Changed $url from http://prism.si.edu/sil/researchbib/srb_search_export_action.cfm to
	http://staff.research.si.edu/srb_search_export_action.cfm. [This the preferred URL.]

~ line 107: Changed format from 'full_html' to '2'. [This is because my test website was migrated from Drupal 6 to D7.
	In D6 the full HTML format is coded as '2'. A D7 website that starts out as a D7 website codes the full HTML format as 'full_html'.]

~ lines 158-163: Added code to have Altmetric badges added to each recent paper in each scientist node.

~ lines 227-229: Edited lines of code to make sure Altmetric badges are not duplicated on All Papers page.