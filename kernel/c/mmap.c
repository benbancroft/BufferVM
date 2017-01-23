//
// Created by ben on 27/12/16.
//

#include "../../libc/stdlib.h"
#include "../../common/syscall.h"
#include "../../common/paging.h"
#include "../h/kernel.h"
#include "../h/vma.h"
#include "../h/stack.h"
#include "../h/utils.h"
#include "../h/host.h"

static uint64_t find_next_gap(uint64_t low_range, uint64_t high_range, uint64_t length) {
    vm_area_t *vma;
    uint64_t input_low_range, input_high_range, gap_start, gap_end;

    input_low_range = low_range;
    input_high_range = high_range;

    gap_end = high_range;
    if (gap_end < length)
        return -ENOMEM;
    high_range = gap_end - length;

    if (low_range > high_range)
        return -ENOMEM;
    low_range = low_range + length;

    //search for gap, starting at highest address and working down
    gap_start = vma_highest_addr;
    if (gap_start <= high_range)
        goto found_highest;

    //check root node
    if (vma_rb_root.rb_node == NULL)
        return -ENOMEM;

    vma = container_of(vma_rb_root.rb_node, vm_area_t, rb_node);
    if (vma->rb_subtree_gap < length)
        return -ENOMEM;

    while (true) {
        gap_start = vma->prev ? vma->prev->end_addr : 0;
        if (gap_start <= high_range && vma->rb_node.rb_right) {
            vm_area_t *right =
                    container_of(vma->rb_node.rb_right, vm_area_t, rb_node);
            if (right->rb_subtree_gap >= length) {
                vma = right;
                continue;
            }
        }

        check_current:
        //is there a gap at the current node?
        gap_end = vma->start_addr;
        if (gap_end < low_range)
            return -ENOMEM;
        if (gap_start <= high_range && gap_end - gap_start >= length)
            goto found;

        if (vma->rb_node.rb_left) {
            vm_area_t *left =
                    container_of(vma->rb_node.rb_left, vm_area_t, rb_node);
            if (left->rb_subtree_gap >= length) {
                vma = left;
                continue;
            }
        }

        //find next node back up rbtree
        while (true) {
            rb_node_t *prev = &vma->rb_node;
            if (!rb_parent(prev))
                return -ENOMEM;
            vma = container_of(rb_parent(prev), vm_area_t, rb_node);
            if (prev == vma->rb_node.rb_right) {
                gap_start = vma->prev ?

                            vma->prev->end_addr : 0;
                goto check_current;
            }
        }
    }

    found:
    if (gap_end > input_high_range)
        gap_end = input_high_range;

    found_highest:
    //calc highest gap address
    gap_end -= length;

    ASSERT(gap_end >= input_low_range);
    ASSERT(gap_end >= gap_start);

    return gap_end;
}

static uint64_t get_unmapped_area_unchecked(uint64_t addr, uint64_t length, uint64_t offset, uint64_t flags) {
    vm_area_t *vma;

    if (!(flags & MAP_FIXED)) {
        //address hint present
        if (addr) {
            addr = PAGE_ALIGN(addr);
            vma = vma_find(addr);
            if (kernel_min_address - length >= addr &&
                (!vma || addr + length <= vma->start_addr))
                return addr;
        } else {
            return find_next_gap(0, user_stack_min, length);
        }
    } else
        return addr;
}

static uint64_t get_unmapped_area(uint64_t addr, uint64_t length, uint64_t offset, uint64_t flags) {

    if (flags & MAP_FIXED)
        return addr;

    addr = get_unmapped_area_unchecked(addr, length, offset, flags);

    if (addr > kernel_min_address - length)
        return -ENOMEM;

    if (addr & ~PAGE_MASK)
        return -EINVAL;

    return addr;
}

int vma_find_links(uint64_t addr,
                   uint64_t end, vm_area_t **pprev,
                   rb_node_t ***rb_link, rb_node_t **rb_parent) {
    rb_node_t **__rb_link, *__rb_parent, *rb_prev;

    __rb_link = &vma_rb_root.rb_node;
    rb_prev = __rb_parent = NULL;

    while (*__rb_link) {
        vm_area_t *vma_tmp;

        __rb_parent = *__rb_link;
        vma_tmp = container_of(__rb_parent, vm_area_t, rb_node);

        if (vma_tmp->end_addr > addr) {
            /* Fail if an existing vma overlaps the area */
            if (vma_tmp->start_addr < end)
                return -ENOMEM;
            __rb_link = &__rb_parent->rb_left;
        } else {
            rb_prev = __rb_parent;
            __rb_link = &__rb_parent->rb_right;
        }
    }

    *pprev = NULL;
    if (rb_prev)
        *pprev = container_of(rb_prev, vm_area_t, rb_node);
    *rb_link = __rb_link;
    *rb_parent = __rb_parent;
    return 0;
}

static void vma_link_list(vm_area_t *vma,
                          vm_area_t *prev, rb_node_t *rb_parent) {
    vm_area_t *next;

    vma->prev = prev;
    if (prev) {
        next = prev->next;
        prev->next = vma;
    } else {
        //keep pointer to list start, for walking purposes
        vma_list_start = vma;
        if (rb_parent)
            next = container_of(rb_parent, vm_area_t, rb_node);
        else
            next = NULL;
    }
    vma->next = next;
    if (next)
        next->prev = vma;
}

static long vma_compute_subtree_gap(vm_area_t *vma) {
    uint64_t max, subtree_gap;
    max = vma->start_addr;
    if (vma->prev)
        max -= vma->prev->end_addr;
    if (vma->rb_node.rb_left) {
        subtree_gap = container_of(vma->rb_node.rb_left, vm_area_t, rb_node)->rb_subtree_gap;
        if (subtree_gap > max)
            max = subtree_gap;
    }
    if (vma->rb_node.rb_right) {
        subtree_gap = container_of(vma->rb_node.rb_right, vm_area_t, rb_node)->rb_subtree_gap;
        if (subtree_gap > max)
            max = subtree_gap;
    }
    return max;
}

static inline void
vma_rb_gap_propagate(rb_node_t *rb, rb_node_t *stop) {
    while (rb != stop) {
        vm_area_t *node = container_of(rb, vm_area_t, rb_node);
        uint64_t augmented = vma_compute_subtree_gap(node);
        if (node->rb_subtree_gap == augmented)
            break;
        node->rb_subtree_gap = augmented;
        rb = rb_parent(&node->rb_node);
    }
}

static void
vma_rb_gap_rotate(rb_node_t *rb_old, rb_node_t *rb_new) {
    vm_area_t *old = container_of(rb_old, vm_area_t, rb_node);
    vm_area_t *new = container_of(rb_new, vm_area_t, rb_node);
    new->rb_subtree_gap = old->rb_subtree_gap;
    old->rb_subtree_gap = vma_compute_subtree_gap(old);
}

static inline void
vma_rb_gap_copy(rb_node_t *rb_old, rb_node_t *rb_new) {
    vm_area_t *old = container_of(rb_old, vm_area_t, rb_node);
    vm_area_t *new = container_of(rb_new, vm_area_t, rb_node);
    new->rb_subtree_gap = old->rb_subtree_gap;
}

void vma_gap_update(vm_area_t *vma) {
    vma_rb_gap_propagate(&vma->rb_node, NULL);
}

//move this later!
static inline void rb_link_node(rb_node_t *node, rb_node_t *parent,
                                rb_node_t **rb_link) {
    node->__rb_parent_color = (uint64_t) parent;
    node->rb_left = node->rb_right = NULL;

    *rb_link = node;
}

static inline rb_node_t *rb_red_parent(rb_node_t *red) {
    return (rb_node_t *) red->__rb_parent_color;
}

static inline void rb_set_parent_color(rb_node_t *rb,
                                       rb_node_t *p, int color) {
    rb->__rb_parent_color = (uint64_t) p | color;
}

static inline void
__rb_change_child(rb_node_t *old, rb_node_t *new,
                  rb_node_t *parent, rb_root_t *root) {
    if (parent) {
        if (parent->rb_left == old)
            parent->rb_left = new;
        else
            parent->rb_right = new;
    } else
        root->rb_node = new;
}

static inline void
__rb_rotate_set_parents(rb_node_t *old, rb_node_t *new,
                        rb_root_t *root, int color) {
    rb_node_t *parent = rb_parent(old);
    new->__rb_parent_color = old->__rb_parent_color;
    rb_set_parent_color(old, new, color);
    __rb_change_child(old, new, parent, root);
}

static inline void rb_set_parent(struct rb_node *rb, struct rb_node *p) {
    rb->__rb_parent_color = rb_color(rb) | (uint64_t) p;
}

static inline void
rb_insert(rb_node_t *node, rb_root_t *root) {
    rb_node_t *parent = rb_red_parent(node), *gparent, *tmp;

    while (true) {
        /*
         * Loop invariant: node is red
         *
         * If there is a black parent, we are done.
         * Otherwise, take some corrective action as we don't
         * want a red root or two consecutive red nodes.
         */
        if (!parent) {
            rb_set_parent_color(node, NULL, RB_BLACK);
            break;
        } else if (rb_is_black(parent))
            break;

        gparent = rb_red_parent(parent);

        tmp = gparent->rb_right;
        if (parent != tmp) {    /* parent == gparent->rb_left */
            if (tmp && rb_is_red(tmp)) {
                /*
                 * Case 1 - color flips
                 *
                 *       G            g
                 *      / \          / \
                 *     p   u  -->   P   U
                 *    /            /
                 *   n            n
                 *
                 * However, since g's parent might be red, and
                 * 4) does not allow this, we need to recurse
                 * at g.
                 */
                rb_set_parent_color(tmp, gparent, RB_BLACK);
                rb_set_parent_color(parent, gparent, RB_BLACK);
                node = gparent;
                parent = rb_parent(node);
                rb_set_parent_color(node, parent, RB_RED);
                continue;
            }

            tmp = parent->rb_right;
            if (node == tmp) {
                /*
                 * Case 2 - left rotate at parent
                 *
                 *      G             G
                 *     / \           / \
                 *    p   U  -->    n   U
                 *     \           /
                 *      n         p
                 *
                 * This still leaves us in violation of 4), the
                 * continuation into Case 3 will fix that.
                 */
                tmp = node->rb_left;
                parent->rb_right = tmp;
                node->rb_left = parent;
                if (tmp)
                    rb_set_parent_color(tmp, parent,
                                        RB_BLACK);
                rb_set_parent_color(parent, node, RB_RED);
                vma_rb_gap_rotate(parent, node);
                parent = node;
                tmp = node->rb_right;
            }

            /*
             * Case 3 - right rotate at gparent
             *
             *        G           P
             *       / \         / \
             *      p   U  -->  n   g
             *     /                 \
             *    n                   U
             */
            gparent->rb_left = tmp; /* == parent->rb_right */
            parent->rb_right = gparent;
            if (tmp)
                rb_set_parent_color(tmp, gparent, RB_BLACK);
            __rb_rotate_set_parents(gparent, parent, root, RB_RED);
            vma_rb_gap_rotate(gparent, parent);
            break;
        } else {
            tmp = gparent->rb_left;
            if (tmp && rb_is_red(tmp)) {
                /* Case 1 - color flips */
                rb_set_parent_color(tmp, gparent, RB_BLACK);
                rb_set_parent_color(parent, gparent, RB_BLACK);
                node = gparent;
                parent = rb_parent(node);
                rb_set_parent_color(node, parent, RB_RED);
                continue;
            }

            tmp = parent->rb_left;
            if (node == tmp) {
                /* Case 2 - right rotate at parent */
                tmp = node->rb_right;
                parent->rb_left = tmp;
                node->rb_right = parent;
                if (tmp)
                    rb_set_parent_color(tmp, parent,
                                        RB_BLACK);
                rb_set_parent_color(parent, node, RB_RED);
                vma_rb_gap_rotate(parent, node);
                parent = node;
                tmp = node->rb_left;
            }

            /* Case 3 - left rotate at gparent */
            gparent->rb_right = tmp; /* == parent->rb_left */
            parent->rb_left = gparent;
            if (tmp)
                rb_set_parent_color(tmp, gparent, RB_BLACK);
            __rb_rotate_set_parents(gparent, parent, root, RB_RED);
            vma_rb_gap_rotate(gparent, parent);
            break;
        }
    }
}

static inline rb_node_t *
rb_erase(rb_node_t *node, rb_root_t *root) {
    struct rb_node *child = node->rb_right;
    struct rb_node *tmp = node->rb_left;
    struct rb_node *parent, *rebalance;
    uint64_t pc;

    if (!tmp) {
        /*
         * Case 1: node to erase has no more than 1 child (easy!)
         *
         * Note that if there is one child it must be red due to 5)
         * and node must be black due to 4). We adjust colors locally
         * so as to bypass __rb_erase_color() later on.
         */
        pc = node->__rb_parent_color;
        parent = __rb_parent(pc);
        __rb_change_child(node, child, parent, root);
        if (child) {
            child->__rb_parent_color = pc;
            rebalance = NULL;
        } else
            rebalance = __rb_is_black(pc) ? parent : NULL;
        tmp = parent;
    } else if (!child) {
        /* Still case 1, but this time the child is node->rb_left */
        tmp->__rb_parent_color = pc = node->__rb_parent_color;
        parent = __rb_parent(pc);
        __rb_change_child(node, tmp, parent, root);
        rebalance = NULL;
        tmp = parent;
    } else {
        struct rb_node *successor = child, *child2;

        tmp = child->rb_left;
        if (!tmp) {
            /*
             * Case 2: node's successor is its right child
             *
             *    (n)          (s)
             *    / \          / \
             *  (x) (s)  ->  (x) (c)
             *        \
             *        (c)
             */
            parent = successor;
            child2 = successor->rb_right;

            vma_rb_gap_copy(node, successor);
        } else {
            /*
             * Case 3: node's successor is leftmost under
             * node's right child subtree
             *
             *    (n)          (s)
             *    / \          / \
             *  (x) (y)  ->  (x) (y)
             *      /            /
             *    (p)          (p)
             *    /            /
             *  (s)          (c)
             *    \
             *    (c)
             */
            do {
                parent = successor;
                successor = tmp;
                tmp = tmp->rb_left;
            } while (tmp);
            child2 = successor->rb_right;
            parent->rb_left = child2;
            successor->rb_right = child;
            rb_set_parent(child, successor);

            vma_rb_gap_copy(node, successor);
            vma_rb_gap_propagate(parent, successor);
        }

        tmp = node->rb_left;
        successor->rb_left = tmp;
        rb_set_parent(tmp, successor);

        pc = node->__rb_parent_color;
        tmp = __rb_parent(pc);
        __rb_change_child(node, successor, tmp, root);

        if (child2) {
            successor->__rb_parent_color = pc;
            rb_set_parent_color(child2, parent, RB_BLACK);
            rebalance = NULL;
        } else {
            uint64_t pc2 = successor->__rb_parent_color;
            successor->__rb_parent_color = pc;
            rebalance = __rb_is_black(pc2) ? parent : NULL;
        }
        tmp = successor;
    }

    vma_rb_gap_propagate(tmp, NULL);
    return rebalance;
}


void vma_link_rb(vm_area_t *vma, rb_node_t **rb_link, rb_node_t *rb_parent) {
    /* Update tracking information for the gap following the new vma. */
    if (vma->next)
        vma_gap_update(vma->next);
    else
        vma_highest_addr = vma->end_addr;

    /*
     * vma->prev wasn't known when we followed the rbtree to find the
     * correct insertion point for that vma. As a result, we could not
     * update the vma rb_node parents rb_subtree_gap values on the way down.
     * So, we first insert the vma with a zero rb_subtree_gap value
     * (to be consistent with what we did on the way down), and then
     * immediately update the gap to the correct value. Finally we
     * rebalance the rbtree after all augmented values have been set.
     */
    rb_link_node(&vma->rb_node, rb_parent, rb_link);
    vma->rb_subtree_gap = 0;
    vma_gap_update(vma);
    rb_insert(&vma->rb_node, &vma_rb_root);
}

static void vma_link(vm_area_t *vma, vm_area_t *prev, rb_node_t **rb_link, rb_node_t *rb_parent) {
    vma_link_list(vma, prev, rb_parent);
    vma_link_rb(vma, rb_link, rb_parent);
}

static void
detach_vmas_to_be_unmapped(vm_area_t *vma,
                           vm_area_t *prev, uint64_t end) {
    vm_area_t **insertion_point;
    vm_area_t *tail_vma = NULL;

    insertion_point = (prev ? &prev->next : &vma_list_start);
    vma->prev = NULL;
    do {
        rb_erase(&vma->rb_node, &vma_rb_root);
        tail_vma = vma;
        vma = vma->next;
    } while (vma && vma->start_addr < end);
    *insertion_point = vma;
    if (vma) {
        vma->prev = prev;
        vma_gap_update(vma);
    } else
        vma_highest_addr = prev ? prev->end_addr : 0;
    tail_vma->next = NULL;
}

static void vma_insert(vm_area_t *vma) {
    vm_area_t *prev;
    rb_node_t **rb_link, *rb_parent;

    if (vma_find_links(vma->start_addr, vma->end_addr,
                       &prev, &rb_link, &rb_parent))
        ASSERT(false);
    vma_link(vma, prev, rb_link, rb_parent);
    //mm->map_count++;
}

static inline void
vma_unlink(vm_area_t *vma, vm_area_t *prev) {
    vm_area_t *next;

    rb_erase(&vma->rb_node, &vma_rb_root);
    prev->next = next = vma->next;
    if (next)
        next->prev = prev;
}

static vm_area_t *vma_remove(vm_area_t *vma) {
    if (vma->file_info.fd != -1)
        host_close(vma->file_info.fd);
    size_t pages = PAGE_DIFFERENCE(vma->end_addr, vma->start_addr);
    printf("Unmapped Pages in range: %p to %p\n", vma->start_addr, vma->end_addr);
    for (size_t i = 0; i < pages; i++){
        unmap_physical_page(vma->start_addr+i*PAGE_SIZE, 0);
    }
    vma_free(vma);
    return vma->next;
}

static void vma_list_remove(vm_area_t *vma) {
    do {
        vma = vma_remove(vma);
    } while (vma);
}

int vma_adjust(vm_area_t *vma, uint64_t start, uint64_t end, uint64_t pgoff, vm_area_t *insert) {
    vm_area_t *next = vma->next;
    bool start_changed = false, end_changed = false;
    long adjust_next = 0;
    int remove_next = 0;

    if (next && !insert) {
        if (end >= next->end_addr) {
            /*
             * vma expands, overlapping all the next, and
             * perhaps the one after too (mprotect case 6).
             */
            remove_next = 1 + (end > next->end_addr);
            end = next->end_addr;

        } else if (end > next->start_addr) {
            /*
             * vma expands, overlapping part of the next:
             * mprotect case 5 shifting the boundary up.
             */
            adjust_next = PAGE_DIFFERENCE(end, next->start_addr);
        } else if (end < vma->end_addr) {
            /*
             * vma shrinks, and !insert tells it's not
             * split_vma inserting another: so it must be
             * mprotect case 4 shifting the boundary down.
             */
            adjust_next = -(PAGE_DIFFERENCE(vma->end_addr, end));
        }
    }
    again:

    if (start != vma->start_addr) {
        vma->start_addr = start;
        start_changed = true;
    }
    if (end != vma->end_addr) {
        vma->end_addr = end;
        end_changed = true;
    }
    vma->page_offset = pgoff;
    if (adjust_next) {
        next->start_addr += adjust_next << PAGE_SHIFT;
        next->page_offset += adjust_next;
    }

    if (remove_next) {
        /*
         * vma_merge has merged next into vma, and needs
         * us to remove next before dropping the locks.
         */
        vma_unlink(next, vma);
    } else if (insert) {
        /*
         * split_vma has split insert from vma, and needs
         * us to insert it before dropping the locks
         * (it may either follow vma or precede it).
         */
        vma_insert(insert);
    } else {
        if (start_changed)
            vma_gap_update(vma);
        if (end_changed) {
            if (!next)
                vma_highest_addr = end;
            else if (!adjust_next)
                vma_gap_update(next);
        }
    }

    if (remove_next) {

        vma_remove(next);
        /*
         * In mprotect's case 6 (see comments on vma_merge),
         * we must remove another next too. It would clutter
         * up the code too much to do both in one go.
         */
        next = vma->next;
        if (remove_next == 2) {
            remove_next = 1;
            end = next->end_addr;
            goto again;
        } else if (next)
            vma_gap_update(next);
        else
            vma_highest_addr = end;
    }

    return 0;
}

static inline int vma_can_merge(vm_area_t *vma, file_t *file_info, uint64_t vm_flags) {
    //TODO - check this covers everything
    // will need to add a check with fcntl F_GETFL to heuristically test if open flags are the same for the fd
    if (vma->file_info.dev != file_info->dev || vma->file_info.inode != file_info->inode)
        return 0;
    return 1;
}


static int
can_vma_merge_before(vm_area_t *vma, uint64_t vm_flags, file_t *file_info, uint64_t vm_pgoff) {
    if (vma_can_merge(vma, file_info, vm_flags)) {
        if (vma->page_offset == vm_pgoff)
            return 1;
    }
    return 0;
}

static int
can_vma_merge_after(vm_area_t *vma, uint64_t vm_flags, file_t *file_info, uint64_t vm_pgoff) {
    if (vma_can_merge(vma, file_info, vm_flags)) {
        uint64_t vm_pglen;
        vm_pglen = file_info->fd != -1 ? PAGE_DIFFERENCE(vma->end_addr, vma->start_addr) : 0;
        if (vma->page_offset + vm_pglen == vm_pgoff)
            return 1;
    }
    return 0;
}

vm_area_t *
vma_merge(vm_area_t *prev, uint64_t addr, uint64_t end, uint64_t vm_flags, file_t *file_info, uint64_t pgoff) {
    uint64_t pglen = file_info->fd != -1 ? PAGE_DIFFERENCE(end, addr) : 0;
    vm_area_t *area, *next;
    int err;

    if (prev)
        next = prev->next;
    else
        next = vma_list_start;
    area = next;
    if (next && next->end_addr == end)        /* cases 6, 7, 8 */
        next = next->next;

    /*
     * Can it merge with the predecessor?
     */
    if (prev && prev->end_addr == addr &&
        can_vma_merge_after(prev, vm_flags, file_info, pgoff)) {
        /*
         * OK, it can.  Can we now merge in the successor as well?
         */
        if (next && end == next->start_addr &&
            can_vma_merge_before(next, vm_flags, file_info, pgoff + pglen)) {
            /* cases 1, 6 */
            err = vma_adjust(prev, prev->start_addr, next->end_addr, prev->page_offset, NULL);
        } else                    /* cases 2, 5, 7 */
            err = vma_adjust(prev, prev->start_addr, end, prev->page_offset, NULL);
        if (err)
            return NULL;
        return prev;
    }

    /*
     * Can this new request be merged in front of next?
     */
    if (next && end == next->start_addr &&
        can_vma_merge_before(next, vm_flags,
                             file_info, pgoff + pglen)) {
        if (prev && addr < prev->end_addr)    /* case 4 */
            err = vma_adjust(prev, prev->start_addr,
                             addr, prev->page_offset, NULL);
        else                    /* cases 3, 8 */
            err = vma_adjust(area, addr, next->end_addr,
                             next->page_offset - pglen, NULL);
        if (err)
            return NULL;
        return area;
    }

    return NULL;
}

static int vma_split(vm_area_t *vma, uint64_t addr, int new_below) {
    vm_area_t *new;
    int err;

    new = vma_zalloc();
    if (!new)
        return -ENOMEM;

    //copy contents
    *new = *vma;

    if (new_below)
        new->end_addr = addr;
    else {
        new->start_addr = addr;
        new->page_offset += PAGE_DIFFERENCE(addr, vma->start_addr);
    }

    //dup file descriptor
    new->file_info.fd = host_dup(new->file_info.fd);

    if (new_below)
        err = vma_adjust(vma, addr, vma->end_addr, vma->page_offset + PAGE_DIFFERENCE(addr, new->start_addr), new);
    else
        err = vma_adjust(vma, vma->start_addr, addr, vma->page_offset, new);

    if (!err)
        return 0;

    //on error, we need to clean up a few things - including the dup'd fd
    if (vma->file_info.fd != -1)
        host_close(new->file_info.fd);
    vma_free(new);
    return err;
}

int syscall_munmap(uint64_t addr, size_t length) {

    printf("munmap\n");
    uint64_t end;
    vm_area_t *vma, *prev, *last;

    if (addr & ~PAGE_MASK || addr > kernel_min_address || length > kernel_min_address - addr || length == 0)
        return -EINVAL;

    //align length to page interval like mmap
    length = PAGE_ALIGN(length);

    vma = vma_find(addr);

    if (!vma)
        return 0;

    prev = vma->prev;

    end = addr + length;

    /*
     * Confirm bounds
     * TODO - move into vma_find?
     */
    if (vma->start_addr >= end)
        return 0;

    //is the address past the start vma (of defined region)
    if (addr > vma->start_addr) {
        int error;

        //check for sysctl_max_map_count
        //switch for null check on allocation for split.
        /*if (end < vma->end_addr)
            return -ENOMEM;*/

        error = vma_split(vma, addr, 0);
        if (error)
            return error;
        prev = vma;
    }

    //is the end past the end vma
    last = vma_find(end);
    if (last && end > last->start_addr) {
        int error = vma_split(last, end, 1);
        if (error)
            return error;
    }
    vma = prev ? prev->next : vma_list_start;

    detach_vmas_to_be_unmapped(vma, prev, end);
    //now we can walk the unlinked list of VMAs to remove/free any resources
    vma_list_remove(vma);

    return 0;


}

uint64_t
mmap_region(file_t *file_info, uint64_t addr, uint64_t length, uint64_t vma_flags, uint64_t vma_prot, uint64_t offset,
            vm_area_t **vma_out) {

    vm_area_t *vma, *prev, *next;
    rb_node_t **rb_link, *rb_parent;

    while (vma_find_links(addr, addr + length, &prev, &rb_link,
                          &rb_parent)) {
        if (syscall_munmap(addr, length)) {
            *vma_out = NULL;
            return -ENOMEM;
        }
    }

    //lets try and merge with an applicable region
    //see can_vma_merge_before and can_vma_merge_after for criteria
    vma = vma_merge(prev, addr, addr + length, vma_flags, file_info, offset);

    if (vma == NULL) {
        vma = vma_zalloc();

        vma->start_addr = addr;
        vma->end_addr = addr + length;
        vma->flags = vma_flags;
        vma->page_offset = offset;
        vma->page_prot = vma_prot;
        vma->file_info = *file_info;

        vma_link(vma, prev, rb_link, rb_parent);
    }

    *vma_out = vma;

    return addr;
}

static inline uint64_t prot_to_vma(uint64_t prot) {
    return TRANSFER_FLAG(prot, PROT_READ, VMA_READ) |
           TRANSFER_FLAG(prot, PROT_WRITE, VMA_WRITE) |
           TRANSFER_FLAG(prot, PROT_EXEC, VMA_EXEC);
}

uint64_t vma_prot_to_pg(uint64_t prot) {
    return TRANSFER_FLAG(prot, VMA_READ, PDE64_PRESENT) |
           TRANSFER_FLAG(prot, VMA_WRITE, PDE64_WRITEABLE) |
           TRANSFER_INV_FLAG(prot, VMA_EXEC, PDE64_NO_EXE) |
           TRANSFER_INV_FLAG(prot, VMA_IS_VERSIONED, PDE64_USER) ;
}

/*
 * Combine the mmap "flags" argument into "flags" used internally.
 */
static inline uint64_t
flag_to_vma(uint64_t flags) {
    return TRANSFER_FLAG(flags, MAP_GROWSDOWN, VMA_GROWS);
}

uint64_t syscall_mmap(uint64_t addr, size_t length, uint64_t prot, uint64_t flags, int fd, uint64_t offset) {

    //should anonymous mappings be done like sbrk?

    //check for map_fixed
    //if not, largely ignore the hint
    //if hint is null, ignore completely
    //else, force it past the end of the heap region for user (MAX_HEAP anyone?)
    //should force any hint to be bigger than brk (unless MAP_FIXED)

    //hack for now until mprotect
    //prot |= PROT_READ | PROT_WRITE | PROT_EXEC;

    vm_area_t *vma;
    int64_t phys_addr;
    uint64_t host_mmap_ret;
    uint64_t vma_flags = 0;
    uint64_t vma_prot = 0;
    file_t file_info = {-1, -1, -1};
    int64_t org_length = length;

    if (offset & ~PAGE_MASK)
        return -EINVAL;

    //First check inputted length
    if (!length)
        return -EINVAL;

    //then check page aligned length
    length = PAGE_ALIGN(length);
    if (!length)
        return -EINVAL;

    addr = get_unmapped_area(addr, length, offset, flags);

    vma_flags |= flag_to_vma(flags);
    vma_prot |= prot_to_vma(prot) | TRANSFER_FLAG(flags, MAP_ANONYMOUS, VMA_IS_VERSIONED);

    if (fd != -1) {

        fd = host_dup(fd);

        //ASSERT(!IS_ERR_VALUE(fd));

        stat_t stats;
        host_fstat(fd, &stats);

        //store inode for comparison later
        //add extra stuff here as needed?

        file_info.fd = fd;
        file_info.dev = stats.st_dev;
        file_info.inode = stats.st_ino;

        switch (flags & MAP_TYPE) {
            case MAP_SHARED:
                vma_flags |= VMA_SHARED;
            case MAP_PRIVATE:
                break;
            default:
                return -EINVAL;
        }
    } else {
        switch (flags & MAP_TYPE) {
            case MAP_SHARED:
                if (vma_flags & VMA_GROWS)
                    return -EINVAL;

                offset = 0;
                vma_flags |= VMA_SHARED;
                break;
            case MAP_PRIVATE:
                //nothing here
                //offset = addr >> PAGE_SHIFT;

                break;
            default:
                return -EINVAL;
        }
    }

    addr = mmap_region(&file_info, addr, length, vma_flags, vma_prot, offset, &vma);

    //MAP_POPULATE | MAP_NONBLOCK cannot be used together
    if (vma != NULL && (file_info.fd != -1 || (flags & (MAP_POPULATE | MAP_NONBLOCK)) == MAP_POPULATE)) {

        //in the case of files, mappings need to be continuous
        phys_addr = vma_fault(vma, true);

        //printf("virtaddr %p %x %x\n", addr, org_length, offset);

        vma->flags |= VMA_IS_PREFAULTED;

        //mmap physical pages that have been pre-allocated
        if (file_info.fd != -1) {
            ASSERT(file_info.fd < 1000);
            //mmap file into continuous physical pages
            ASSERT(phys_addr != -1);
            host_mmap_ret = (uint64_t) host_mmap((void *) phys_addr, length, prot, flags | MAP_FIXED, fd, offset);
            ASSERT((int64_t) host_mmap_ret == phys_addr);

            //printf("host mmap %p %p\n", host_mmap_ret, phys_addr);
        }
    }

    return addr;
}

int syscall_mprotect(void *addr, size_t len, int prot){
    //TODO
    printf("mprotect %p of len %d with prot %x\n", addr, len, prot);
    return 0;
}
