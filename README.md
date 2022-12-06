# GoalHeatmap
Rocket league plugin rendering heatmap of goals


I initially made this plugin to see if I had to train defense when the ball arrive on sides of the map.
By creating a heatmap of ball positions up to 10 seconds before a goal (you can choose), players can see if a particular position on the map is frequently less defended or too much favored when attacking.
Data is registered to a txt file each time a goal is scored and the heatmap regenerated at the end of each game (or manually via the plugin interface).

Currently every goal scored is registered, so if you play with random teammates in 4v4, it will count too. Anyway, statistically, it will be a good indication of your performance the more you play.
I'll probably update the plugin to add more filters (like game type, dates, etc) but I'm open to suggestions too.

The heatmap is generated using libheatmap - C library:  https://github.com/lucasb-eyer/libheatmap.

Feel free to mp me if you have any question or to report bugs (Peepan#1064 on Discord).
