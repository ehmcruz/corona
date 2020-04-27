#!/usr/bin/env Rscript

library(tidyverse)
library(maditr)
library(reshape)
library(ggplot2)

# https://ggplot2.tidyverse.org/reference/geom_ribbon.html

# https://www.r-graph-gallery.com/104-plot-lines-with-error-envelopes-ggplot2.html

# aes(ymin = mean - se, ymax = mean + se)

# https://pastebin.com/FQFbUEcd

# https://stackoverflow.com/questions/38470111/how-to-graph-with-geom-ribbon

counters <- list("ac_infected_state_ST_MILD", "ac_infected_state_ST_SEVERE", "ac_infected_state_ST_CRITICAL", "ac_state_ST_DEAD")
colors <- c("yellow", "orange", "red", "blue")

args = commandArgs(trailingOnly=TRUE)

if (length(args) != 3)
	stop("Usage: plot-cycles.R <mean.csv> <stddeev.csv> <output_base>\n")

csv_mean = args[1]
csv_error = args[2]
output_base = args[3]

data_mean_ = read.csv(csv_mean, header = TRUE)
data_error_ = read.csv(csv_error, header = TRUE)

i <- 1
for (counter in counters) {
	print(counter)

	data_mean <- data_mean_
	data_error <- data_error_

	data <- data_mean %>% select(cycle)
	data$mean <- data_mean[, counter]
	data$error <- data_error[, counter]

	data$lower_bound <- data$mean - data$error
	data$upper_bound <- data$mean + data$error

	#print(data)
	#stop()

	# h <- ggplot(huron, aes(year))

	# h + geom_ribbon(aes(ymin = level - 1, ymax = level + 1), fill = "grey70") + geom_line(aes(y = level))

	#png(file=paste0(output_base, "-", counter, ".png"))
	pdf(file=paste0(output_base, "-", counter, ".pdf"))

	print(p <- ggplot(data, aes(x =cycle))
		+
	geom_ribbon(aes(ymin=lower_bound, ymax=upper_bound), 
	                 #alpha=0.1,       #transparency
	                 linetype=1,      #solid, dashed or other line types
	                 colour="grey70", #border line color
	                 size=1,          #border line size
	                 fill=colors[i])    #fill color
	)
	i <- i + 1
#p <- ggplot(data, aes(x = Q, y = C, color=Name, group=Name))

# p <- p + geom_line()
# p + geom_ribbon(aes(ymin=Lower_Bound, ymax=Upper_Bound), 
#                 alpha=0.1,       #transparency
#                 linetype=1,      #solid, dashed or other line types
#                 colour="grey70", #border line color
#                 size=1,          #border line size
#                 fill="green")    #fill color
}