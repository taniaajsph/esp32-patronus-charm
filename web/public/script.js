const socket = io();
const videoPlayer = document.getElementById('videoPlayer');

// List of video URLs (place your local or hosted URLs here)
const videos = [
  "cat.mp4",
  "front.mp4",
  "wolf.mp4"
];

socket.on('playRandom', () => {
  const randomIndex = Math.floor(Math.random() * videos.length);
  const selectedVideo = videos[randomIndex];
  videoPlayer.src = selectedVideo;
  videoPlayer.play();
});
