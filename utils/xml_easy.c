
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

char *xml_easy_dump(xmlNodePtr root)
{
    xmlDocPtr xml_doc = NULL;
    xmlChar *out_buf;
    int out_len;

    xml_doc = xmlNewDoc((const xmlChar *)"1.0");
    xmlDocSetRootElement(xml_doc, root);
    xmlDocDumpFormatMemoryEnc(xml_doc, &out_buf, &out_len, "utf8", 1);
    xmlFreeDoc(xml_doc);
    return out_buf;
}

int xml_easy_get_prop(xmlNodePtr root, char *name, char *value, int size)
{
    xmlAttrPtr attr;
    xmlChar *tmp = NULL;

    if (value == NULL || size <= 0) {
        return -1;
    }

    value[0] = 0;
    attr = root->properties;
    while (attr != NULL) {
        if (xmlStrcasecmp(attr->name, name) == 0) {
            tmp = xmlGetProp(root, attr->name);
            break;
        }
        attr = attr->next;
    }
    if (tmp == NULL) {
        return -1;
    }
    snprintf(value, size, "%s", tmp);
    xmlFree(tmp);
    return 0;
}

int xml_easy_get_prop_int(xmlNodePtr root, char *name, int default_value)
{
    int ret;
    char tmp[256];
    int value;
    value = default_value;
    ret = xml_easy_get_prop(root, name, tmp, sizeof(tmp));
    if (ret == 0) {
        value = atoi(tmp);
    }
    return value;
}

xmlNodePtr xml_easy_get_child(xmlNodePtr root, char *name)
{
    xmlNodePtr node;

    node = root->children;
    while (node != NULL) {
        if (xmlStrcasecmp(node->name, name) == 0) {
            return node;
        }
        if (node->next == node) {
            break;
        }
        node = node->next;
    }
    return NULL;
}

xmlNodePtr xml_easy_get_next(xmlNodePtr root, char *name)
{
    xmlNodePtr node;

    node = root->next;
    while (node != NULL) {
        if (xmlStrcasecmp(node->name, name) == 0) {
            return node;
        }
        if (node->next == node) {
            break;
        }
        node = node->next;
    }
    return NULL;
}

int xml_easy_get_content(xmlNodePtr root, char *value, int size)
{
    xmlChar *tmp = NULL;
    tmp = xmlNodeGetContent(root);
    if (tmp == NULL) {
        return -1;
    }
    snprintf(value, size, "%s", tmp);
    xmlFree(tmp);
    return 0;
}

int xml_easy_get_child_content(xmlNodePtr root, char *name, char *value, int size)
{
    xmlNodePtr node;

    if (value == NULL || size <= 0) {
        return -1;
    }
    value[0] = 0;

    node = xml_easy_get_child(root, name);
    if (node == NULL) {
        return -1;
    }
    return xml_easy_get_content(node, value, size);
}

int xml_easy_get_child_content_int(xmlNodePtr root, char *name, int default_value)
{
    int value;
    xmlChar tmp[256];
    int ret;

    value = default_value;
    ret = xml_easy_get_child_content(root, name, tmp, sizeof(tmp));
    if (ret == 0) {
        value = atoi(tmp);
    }
    return value;
}

int xml_easy_get_child_content_boolean(xmlNodePtr root, char *name, int default_value)
{
    int value;
    xmlChar tmp[256];
    int ret;

    value = default_value;
    ret = xml_easy_get_child_content(root, name, tmp, sizeof(tmp));
    if (ret == 0) {
        if (xmlStrcasecmp(tmp, "true") == 0) {
            value = 1;
        } else {
            value = 0;
        }
    }
    return value;
}

int xml_easy_add_prop_int(xmlNodePtr root, char *name, int value)
{
    char tmp[256];

    snprintf(tmp, sizeof(tmp) - 1, "%d", value);
    xmlNewProp(root, name, tmp);
    return 0;
}

xmlNodePtr xml_easy_new_node(xmlNodePtr root, char *name)
{
    xmlNodePtr node;
    node = xmlNewNode(NULL, name);
    if (node == NULL) {
        return NULL;
    }
    if (root != NULL) {
        xmlAddChild(root, node);
    }
    return node;
}

int xml_easy_add_child_string(xmlNodePtr root, char *name, char *value)
{
    xmlNewTextChild(root, NULL, name, value);
    return 0;
}

int xml_easy_add_child_int(xmlNodePtr root, char *name, int value)
{
    char tmp[256];
    snprintf(tmp, sizeof(tmp) - 1, "%d", value);
    xmlNewTextChild(root, NULL, name, tmp);
    return 0;
}

int xml_easy_add_child_boolean(xmlNodePtr root, char *name, int value)
{
    if (value == 0) {
        xmlNewTextChild(root, NULL, name, "false");
    } else {
        xmlNewTextChild(root, NULL, name, "true");
    }

    return 0;
}
