/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup LIST LIST
 * @ingroup  COMMON
 * @brief    Singly linked list
 * @details  Singly linked list
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __OM_LIST_H
#define __OM_LIST_H


/*******************************************************************************
 * INCLUDES
 */
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * MACROS
 */
/**
 *******************************************************************************
 * @brief traverse all node
 *
 * @param[in] pos       current node
 * @param[in] list      list
 *******************************************************************************
 */
#define om_list_for_each(pos, list) \
    for (pos = (list)->head; pos != NULL; pos = pos->next)

/**
 *******************************************************************************
 * @brief traverse all node, can delete node
 *
 * @param[in] pos_prev  previous node
 * @param[in] pos       current node
 * @param[in] list      list
 *******************************************************************************
 */
#define om_list_for_each_safe(pos_prev, pos, list) \
    for (pos_prev = NULL, pos = (list)->head; pos != NULL; pos_prev = pos, pos = pos->next)


/*******************************************************************************
 * TYPEDEFS
 */
/// list node define
typedef struct om_list_node {
    /// next node
    struct om_list_node * volatile next;
} om_list_node_t;

/// list define
typedef struct {
    /// head node
    struct om_list_node * volatile head;
    /// tail node
    struct om_list_node * volatile tail;
} om_list_t;

/**
 *******************************************************************************
 * @brief decide whether insert new node by comparision
 *
 * @param[in] new_node      new insert node
 * @param[in] cur_node      current node
 * @param[in] param         param
 *
 * @return whether insert
 *******************************************************************************
 */
typedef bool (*om_list_cmp_insert_callback_t)(const om_list_node_t *new_node, const om_list_node_t *cur_node, void *param);

/**
 *******************************************************************************
 * @brief decide whether extract current node
 *
 * @param[in] cur_node      current node
 * @param[in] param         param
 *
 * @return whether extract
 *******************************************************************************
 */
typedef bool (*om_list_cmp_extract_callback_t)(const om_list_node_t *cur_node, void *param);


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief om list init
 *
 * @param[in] list    list
 *******************************************************************************
 */
extern void om_list_init(om_list_t *list);

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
extern bool om_list_is_empty(const om_list_t *list);

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
extern bool om_list_node_is_first(const om_list_node_t *node, const om_list_t *list);

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
extern om_list_node_t *om_list_get_first_node(om_list_t *list);

/**
 *******************************************************************************
 * @brief get the last node of list
 *
 * @param[in] list    list
 *
 * @retval NULL  no item
 * @retval other  the pointer to node
 *******************************************************************************
 */
extern om_list_node_t *om_list_get_last_node(om_list_t *list);

/**
 *******************************************************************************
 * @brief move one node from old list to new list's tail
 *
 * @param[in] node          the node to be moved
 * @param[in] old_list      the old list
 * @param[in] new_list      the new list
 *******************************************************************************
 */
extern void om_list_move(om_list_node_t *node, om_list_t *old_list, om_list_t *new_list);

/**
 *******************************************************************************
 * @brief del node
 *
 * @param[in] node     node to be deleted
 * @param[in] list     list
 *
 * @retval true  deleted successfully
 * @retval false  deleted faily
 *******************************************************************************
 */
extern bool om_list_del_node(om_list_node_t *node, om_list_t *list);

/**
 * @brief  om list add front
 *
 * @param[in] node  node
 * @param[in] list  list
 **/
extern void om_list_add_front(om_list_node_t *node, om_list_t *list);

/**
 * @brief  om list add
 *
 * @param[in] node  node
 * @param[in] list  list
 **/
extern void om_list_add(om_list_node_t *node, om_list_t *list);

/**
 *******************************************************************************
 * @brief list push
 *
 * @param[in] node    the node to be push
 * @param[in] list    list
 *******************************************************************************
 */
extern void om_list_push(om_list_node_t *node, om_list_t *list);

/**
 *******************************************************************************
 * @brief list push to front
 *
 * @param[in] node    the node to be push
 * @param[in] list    list
 *******************************************************************************
 */
extern void om_list_push_front(om_list_node_t *node, om_list_t *list);

/**
 * @brief  om list pop
 *
 * @param[in] list  list
 *
 * @return  the poped node
 **/
extern om_list_node_t *om_list_pop(om_list_t *list);

/**
 *******************************************************************************
 * @brief list pop behind
 *
 * @param[in] list    list
 *
 * @retval NULL  no node
 * @retval OTHER  the pointer to node
 *******************************************************************************
 */
extern om_list_node_t *om_list_pop_behind(om_list_t *list);

/**
 * @brief  om list num
 *
 * @param[in] list  list
 *
 * @return number
 **/
extern unsigned om_list_num(const om_list_t *list);

/**
 *******************************************************************************
 * @brief  om list extract
 *
 * @param[in] list  list
 * @param[in] list_hdr  list hdr
 *
 * @return extract ?
 *******************************************************************************
 **/
extern bool om_list_extract(om_list_t *list, om_list_node_t *list_hdr);

/**
 *******************************************************************************
 * @brief extrac a node
 *
 * if extract_cb() return true, extract it and delete it
 * if no true return, extract NULL
 *
 * @param[in] list          list
 * @param[in] extract_cb    extract callback
 * @param[in] param         extract param
 *
 * @return if find, extract it, otherwise extract NULL
 *******************************************************************************
 */
extern om_list_node_t *om_list_extract_ex(om_list_t *list, om_list_cmp_extract_callback_t extract_cb, void *param);

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
extern bool om_list_find(om_list_t *list, om_list_node_t *list_hdr);

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
extern om_list_node_t *om_list_find_ex(om_list_t *list, om_list_cmp_extract_callback_t extract_cb, void *param);

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
 *******************************************************************************
 */
extern void om_list_insert(om_list_t *list, om_list_node_t *new_node, om_list_cmp_insert_callback_t ins_cb, void *param);

/**
 *******************************************************************************
 * @brief  om list insert before
 *
 * @param[in] list  list
 * @param[in] elt_ref_hdr  elt ref hdr
 * @param[in] elt_to_add_hdr  elt to add hdr
 *******************************************************************************
 **/
extern void om_list_insert_before(om_list_t *list, om_list_node_t *elt_ref_hdr, om_list_node_t *elt_to_add_hdr);

#ifdef __cplusplus
}
#endif

#endif  /* __OM_LIST_H */


/** @} */
