#ifndef _CWMPACS_UTILS_XML_EASY_H_
#define _CWMPACS_UTILS_XML_EASY_H_

#include <libxml/parser.h>
#include <libxml/tree.h>

int xml_easy_dump(xmlNodePtr root);

int xml_easy_get_prop(xmlNodePtr root, char *name, char *value, int size);
int xml_easy_get_prop_int(xmlNodePtr root, char *name, int default_value);

xmlNodePtr xml_easy_get_child(xmlNodePtr root, char *name);
xmlNodePtr xml_easy_get_next(xmlNodePtr root, char *name);
int xml_easy_get_content(xmlNodePtr root, char *value, int size);
int xml_easy_get_child_content(xmlNodePtr root, char *name, char *value, int size);
int xml_easy_get_child_content_int(xmlNodePtr root, char *name, int default_value);
int xml_easy_get_child_content_boolean(xmlNodePtr root, char *name, int default_value);

xmlNodePtr xml_easy_new_node(xmlNodePtr root, char *name);
int xml_easy_add_prop_int(xmlNodePtr root, char *name, int value);
int xml_easy_add_child(xmlNodePtr root, char *name, char *value);
int xml_easy_add_child_int(xmlNodePtr root, char *name, int value);
int xml_easy_add_child_boolean(xmlNodePtr root, char *name, int value);

#endif
