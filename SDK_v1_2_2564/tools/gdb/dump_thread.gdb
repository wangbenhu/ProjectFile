################################################
# Print One Thread TCB Information
################################################
set $taskIndex = 0
define printTCB
    set $taskIndex  += 1
    set $curTaskTCB = (TCB_t *)$arg0

    printf "%02d  ", $taskIndex

    if $curTaskTCB == pxCurrentTCB
        output (eTaskState)0
    else
        output (eTaskState)$arg1
    end
    printf "\t"

    printf "%-14s", $curTaskTCB->pcTaskName
    printf "  "

    printf "0x%x", $curTaskTCB
    printf " "

    printf "0x%x", $curTaskTCB->pxTopOfStack
    printf " "

    printf "0x%x", $curTaskTCB->pxStack
    printf " "

    printf "0x%x", $curTaskTCB->pxEndOfStack
    printf " "

    printf "0x%-4x", ((int)($curTaskTCB->pxEndOfStack) - (int)($curTaskTCB->pxStack) + 8) & (~7)
    printf "        "


    set $pCharStack = (uint8_t *)$curTaskTCB->pxStack
    set $ulConut = 0
    while *($pCharStack)++ == 0xa5
        set $ulConut += 1
    end

    printf "0x%x", $ulConut
    printf "\t\t"

    printf "0x%x\n", $curTaskTCB->uxPriority
end

################################################
# Print Thread Dump Header
################################################
printf "Num State       Name            TCB        StkCruPtr  StkLmt     StkTop     StkTotalSize  StkMinSpace   Pri\n"

################################################
# Print Ready and Running Tasks
################################################
set $indexItem = 0
set $totalItem = sizeof(pxReadyTasksLists) / sizeof(List_t)

set $curItem = pxReadyTasksLists[$indexItem]
set $numNode = $curItem.uxNumberOfItems

while $indexItem < $totalItem

    if $numNode != 0
        printTCB $curItem.xListEnd.pxNext->pvOwner 1
        set $numNode -= 1
        set $curItem = $curItem.xListEnd.pxNext
    else
        set $indexItem += 1
        set $curItem = pxReadyTasksLists[$indexItem]
        set $numNode = $curItem.uxNumberOfItems
    end
end

################################################
# Print Blocked Tasks
################################################
set $curItem = pxDelayedTaskList->xListEnd.pxNext
set $numNode = pxDelayedTaskList->uxNumberOfItems

while $numNode > 0
    printTCB $curItem->pvOwner 2
    set $numNode -= 1
    set $curItem = $curItem->pxNext
end

set $curItem = pxOverflowDelayedTaskList->xListEnd.pxNext
set $numNode = pxOverflowDelayedTaskList->uxNumberOfItems

while $numNode > 0
    printTCB $curItem->pvOwner 2
    set $numNode -= 1
    set $curItem = $curItem->pxNext
end

################################################
# Print Suspended Tasks
################################################
set $curItem = xSuspendedTaskList.xListEnd.pxNext
set $numNode = xSuspendedTaskList.uxNumberOfItems

while $numNode > 0
    set $taskState = 3
    if (((TCB_t *)$curItem->pvOwner)->xEventListItem.pvContainer != 0) || (((TCB_t *)$curItem->pvOwner)->ucNotifyState[0] == 1)
        set $taskState = 2
    end
    printTCB $curItem->pvOwner $taskState
    set $numNode -= 1
    set $curItem = $curItem->pxNext
end

################################################
# Print Deleted Tasks
################################################
set $curItem = xTasksWaitingTermination.xListEnd.pxNext
set $numNode = xTasksWaitingTermination.uxNumberOfItems

while $numNode > 0
    printTCB $curItem->pvOwner 4
    set $numNode -= 1
    set $curItem = $curItem->pxNext
end