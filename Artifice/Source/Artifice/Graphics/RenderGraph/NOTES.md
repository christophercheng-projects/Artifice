# Render Graph

## TODO:
* Modules, groupings of passes that can be added to the graph at once
* ~~Blackboard, global communication among passes~~
* Scheduling, efficiently splitting up work/multithreading
* Interactions with swaphchains and presentation signaling
* Efficient creation/caching semaphores
    * Currently creating and destroying on every construction phase (AddPass)
* Improve line between construction and evaluation
* Subgraphs, run multiple at different frequencies
* ~~MSAA~~


## Scribbles

Group: A sequence of passes, beginning with a start pass, where each subsequent pass is dependent on and only on the previous pass in the sequence, and all passes run on the same queue.

Group dependencies:

Group convergence points: 