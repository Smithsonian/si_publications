SI publications
====================================

This module was written by Anthony Braun at NZP. If you have any questions or want to help improve this module, please email me at brauna@si.edu

### SUMMARY
 The SI Publications module allows you to import publications for your unit's scientists from Smithsonian Libraries. This can be done manually or automatically as a cron task using the Drupal queue API.

    
### REQUIREMENTS
 You must have created a "scientist" content type (it can be named anything you would like). This content type MUST contain the following two fields:
 1. A text field for the scientist's SI Profiles ID. This can be named anything, but it must be a text field.
 2. A longtext field for recent papers. This must allow unlimited values and it must be HTML-enabled. Again, the name does not matter.

### INSTALLATION
 Install as usual, see http://drupal.org/documentation/install/modules-themes/modules-7 for further information

### CONFIGURATION
 Go to /admin/content/publications/settings and fill out the form.