
include config.mk
all:   
#用shell命令for搞，shell里边的变量用两个$
	@for dir in $(BUILD_DIR); \
	do \
		make -C $$dir; \
	done


clean:
#-rf：删除文件夹，强制删除
	rm -rf app/link_obj app/dep nginx
	rm -rf signal/*.gch app/*.gch