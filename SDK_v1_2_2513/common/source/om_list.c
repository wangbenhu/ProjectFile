/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup DOC DOC
 * @ingroup  DOCUMENT
 * @brief    Singly linked list
 * @details  Singly linked list
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */


/*******************************************************************************
 * INCLUDES
 */
#include "om_list.h"
#include "om_utils.h"


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief om list init
 *
 * @param[in] list    list
 *******************************************************************************
 */
void om_list_init(om_list_t *list)
{
    if (list) {
        list->head = NULL;
        list->tail = NULL;
    } else {
        OM_ASSERT(0);
    }
}

/**
 *******************************************************************************
 * @brief whether is empty
 *
 * @param[in] list    list
 *
 * @return
 *    - true:    is empty
 *    - false:   not empty
 *******************************************************************************
 */
bool om_list_is_empty(const om_list_t *list)
{
    if ((list->head == NULL) && (list->tail == NULL)) {
        return true;
    } else if ((list->head != NULL) && (list->tail != NULL)) {
        return false;
    } else {
        OM_ASSERT(0);
        return false;
    }
}

/**
 *******************************************************************************
 * @brief whether node is the first one
 *
 * @param[in] node  check node
 * @param[in] list  list
 *
 * @return
 *    - true:  is first
 *    - false: not first
 *******************************************************************************
 */
bool om_list_node_is_first(const om_list_node_t *node, const om_list_t *list)
{
    return list->head == node;
}

/**
 *******************************************************************************
 * @brief get the first node of list
 *
 * @param[in] list    list
 *
 * @return
 *    - NULL : no node
 *    - the pointer to node
 *******************************************************************************
 */
om_list_node_t *om_list_get_first_node(om_list_t *list)
{
    return om_list_is_empty(list) ? NULL : list->head;
}

/**
 *******************************************************************************
 * @brief get the last node of list
 *
 * @param[in] list    list
 *
 * @return
 *    - NULL : no item
 *    - the pointer to node
 *******************************************************************************
 */
om_list_node_t *om_list_get_last_node(om_list_t *list)
{
    return om_list_is_empty(list) ? NULL : list->tail;
}

/**
 *******************************************************************************
 * @brief move one node from old list to new list's tail
 *
 * @param[in] node          the node to be moved
 * @param[in] old_list      the old list
 * @param[in] new_list      the new list
 *
 * @return None
 *******************************************************************************
 */
void om_list_move(om_list_node_t *node, om_list_t *old_list, om_list_t *new_list)
{
    om_list_del_node(node, old_list);
    om_list_add(node, new_list);
}

/**
 *******************************************************************************
 * @brief del node
 *
 * @param[in] node     node to be deleted
 * @param[in] list     list
 *
 * @return
 *    - true:  deleted successfully
 *    - false: deleted faily
 *******************************************************************************
 */
bool om_list_del_node(om_list_node_t *node, om_list_t *list)
{

    om_list_node_t *prev_node, *cur_node;

    om_list_for_each_safe(prev_node, cur_node, list) {
        if (node == cur_node) {
            if (prev_node == NULL) {        // the first node
                list->head = cur_node->next;
            } else {
                prev_node->next = cur_node->next;
            }
            if (cur_node == list->tail) {  // the last node
                list->tail = prev_node;
            }

            return true;
        }
    }

    return false;
}

/**
 * @brief  om list add front
 *
 * @param[in] node  node
 * @param[in] list  list
 **/
void om_list_add_front(om_list_node_t *node, om_list_t *list)
{
    if ((list != NULL) && (node != NULL)) {
        node->next = list->head;
        list->head = node;
        if (list->tail == NULL) {
            list->tail = node;
        }
    } else {
        OM_ASSERT(0);
    }
}

/**
 * @brief  om list add
 *
 * @param[in] node  node
 * @param[in] list  list
 **/
void om_list_add(om_list_node_t *node, om_list_t *list)
{
    if ((list != NULL) && (node != NULL)) {
        node->next = NULL;
        if (list->tail) {
            list->tail->next = node;
        } else {    // first node
            list->head = node;
        }
        list->tail = node;
    } else {
        OM_ASSERT(0);
    }
}

/**
 *******************************************************************************
 * @brief list push
 *
 * @param[in] node    the node to be push
 * @param[in] list    list
 *
 * @return None
 *******************************************************************************
 */
void om_list_push(om_list_node_t *node, om_list_t *list)
{
    om_list_add(node, list);
}

/**
 *******************************************************************************
 * @brief list push to front
 *
 * @param[in] node    the node to be push
 * @param[in] list    list
 *
 * @return None
 *******************************************************************************
 */
void om_list_push_front(om_list_node_t *node, om_list_t *list)
{
    om_list_add_front(node, list);
}

/**
 * @brief  om list pop
 *
 * @param[in] list  list
 *
 * @return
 **/
om_list_node_t *om_list_pop(om_list_t *list)
{
    om_list_node_t *head_node;

    if (list) {
        head_node = list->head;
        if (list->head) {
            list->head = head_node->next;
            if (list->head == NULL) {
                list->tail = NULL;
            }
        }
        return head_node;
    }

    return NULL;
}

/**
 *******************************************************************************
 * @brief list pop behind
 *
 * @param[in] list    list
 *
 * @return
 *    - NULL : no node
 *    - the pointer to node
 *******************************************************************************
 */
om_list_node_t *om_list_pop_behind(om_list_t *list)
{
    om_list_node_t *tail_node;

    tail_node = om_list_get_last_node(list);

    if (tail_node) {
        om_list_del_node(tail_node, list);
    }

    return tail_node;
}

/**
 * @brief  om list num
 *
 * @param[in] list  list
 *
 * @return
 **/
unsigned om_list_num(const om_list_t *list)
{
    om_list_node_t *tmp_list_hdr;
    unsigned num = 0;

    // Go through the list to find the element
    tmp_list_hdr = list->head;

    while (tmp_list_hdr != NULL) {
        tmp_list_hdr = tmp_list_hdr->next;
        ++num;
    }

    return num;
}

/**
 ****************************************************************************************
 * @brief  om list extract
 *
 * @param[in] list  list
 * @param[in] list_hdr  list hdr
 *
 * @return extract ?
 ****************************************************************************************
 **/
bool om_list_extract(om_list_t *list, om_list_node_t *list_hdr)
{
    bool found = false;

    om_list_node_t *prev = NULL;
    om_list_node_t *curr = list->head;

    // Search for the element
    while(curr != NULL) {
        // Check element
        if(curr == list_hdr) {
            found = true;
            break;
        }

        // Move pointers
        prev = curr;
        curr = curr->next;
    }

    if(found) {
        // Check if the element is first
        if(prev == NULL) {
            // Extract element
            list->head = list_hdr->next;
        } else {
            // Extract element
            prev->next = list_hdr->next;
        }

        // Check if the element is last
        if(list_hdr == list->tail) {
            // Update last pointer
            list->tail = prev;
        }
    }

    return found;
}

/**
 *******************************************************************************
 * @brief extrac a node
 *
 * if extract_cb() return true, extract it and delete it
 * if no true return, extract NULL
 *
 * @param[in] list          list
 * @param[in] extract_cb    extract callback
 *
 * @return if find, extract it, otherwise extract NULL
 *******************************************************************************
 */
om_list_node_t *om_list_extract_ex(om_list_t *list, om_list_cmp_extract_callback_t extract_cb, void *param)
{
    om_list_node_t *cur_node, *prev_node;

    om_list_for_each_safe(prev_node, cur_node, list) {
        if (extract_cb(cur_node, param)) {
            if (prev_node == NULL) {
                list->head = cur_node->next;    // extract the first node
            } else {
                prev_node->next = cur_node->next;
            }

            // check is the node is last
            if (list->tail == cur_node) {
                list->tail = prev_node;
            }
            return cur_node;
        }
    }

    return NULL;
}

/**
 *******************************************************************************
 * @brief  om list find
 *
 * @param[in] list  list
 * @param[in] list_hdr  list hdr
 *
 * @return find?
 *******************************************************************************
 **/
bool om_list_find(om_list_t *list, om_list_node_t *list_hdr)
{
    om_list_node_t *tmp_list_hdr;

    // Go through the list to find the element
    tmp_list_hdr = list->head;

    while ((tmp_list_hdr != list_hdr) && (tmp_list_hdr != NULL)) {
        tmp_list_hdr = tmp_list_hdr->next;
    }

    return (tmp_list_hdr == list_hdr);
}

/**
 *******************************************************************************
 * @brief find a node to be extracted
 *
 * @param[in] list              list
 * @param[in] extract_cb        extract callback
 * @param[in] param             param
 *
 *
 * @return
 *    - the node that was found
 *    - NULL
 *******************************************************************************
 */
om_list_node_t *om_list_find_ex(om_list_t *list, om_list_cmp_extract_callback_t extract_cb, void *param)
{
    om_list_node_t *cur_node;

    om_list_for_each(cur_node, list) {
        if (extract_cb(cur_node, param)) {
            return cur_node;
        }
    }

    return NULL;
}

/**
 *******************************************************************************
 * @brief insert a item to a specified place, the place is decided by ins_cb
 *
 * if ins_cb() return true, insert the front of the cur_node.
 * if no true return ,insert the tail
 *
 * @param[in] list        list
 * @param[in] new_node    the node to be inserted
 * @param[in] ins_cb      insert callback
 * @param[in] param       param
 *
 * @return None
 *******************************************************************************
 */
void om_list_insert(om_list_t *list, om_list_node_t *new_node, om_list_cmp_insert_callback_t ins_cb, void *param)
{
    om_list_node_t *cur_node, *prev_node;

    om_list_for_each_safe(prev_node, cur_node, list) {
        if (ins_cb(new_node, cur_node, param)) {
            if (prev_node == NULL) {
                om_list_add_front(new_node, list);             // list has only one node
            } else {
                new_node->next = cur_node;                     // found suitable cur_node to insert
                prev_node->next = new_node;
            }
            return;
        }
    }
    om_list_add(new_node, list);    // list is NULL or no suitable cur_node to insert
}

/**
 *******************************************************************************
 * @brief  om list insert before
 *
 * @param[in] list  list
 * @param[in] elt_ref_hdr  elt ref hdr
 * @param[in] elt_to_add_hdr  elt to add hdr
 *******************************************************************************
 **/
void om_list_insert_before(om_list_t *list, om_list_node_t *elt_ref_hdr, om_list_node_t *elt_to_add_hdr)
{
    // If no element referenced
    if(elt_ref_hdr == NULL) {
        om_list_add_front(elt_to_add_hdr, list);
    } else {
        om_list_node_t *tmp_list_prev_hdr = NULL;
        om_list_node_t *tmp_list_curr_hdr;

        // Go through the list to find the element
        tmp_list_curr_hdr = list->head;

        while ((tmp_list_curr_hdr != elt_ref_hdr) && (tmp_list_curr_hdr != NULL)) {
            // Save previous element
            tmp_list_prev_hdr = tmp_list_curr_hdr;
            // Get the next element of the list
            tmp_list_curr_hdr = tmp_list_curr_hdr->next;
        }
        // If only one element is available
        if(tmp_list_prev_hdr == NULL) {
            om_list_add_front(elt_to_add_hdr, list);
        } else {
            tmp_list_prev_hdr->next = elt_to_add_hdr;
            elt_to_add_hdr->next = tmp_list_curr_hdr;
        }
    }
}


/** @} */
