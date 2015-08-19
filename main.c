
int main(void)
{
	int fd = -1;
	fd = open_camera();
	if(fd < 0)
		exit(0);
	init_param(fd);

	start_capture_data(fd);

	close_camera(fd);
	return 0;
}
