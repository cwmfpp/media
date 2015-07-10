
int main(void)
{
	int fd = -1;
	fd = open_camera();
	init_param(fd);

	start_capture_data(fd);

	close_camera(fd);
	return 0;
}
