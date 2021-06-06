#include "../Header/Blocks.h"

Blocks::Blocks(Mario& mario, block_t type, float x, float y) {
	// Set initial values
	this->mario = &mario;
	questionRect = IntRect(0, 0, 32, 31);
	stoneRect = IntRect(32, 0, 32, 31);
	bronzeRect = IntRect(0, 32, 32, 31);
	smashRect = IntRect(0, 0, 800, 800);

	currentRect = movingSpeed = stuckRange = 0;
	display = true;
	faid = isPopUp = marioOn = false;
	blockType = type;
	startPos.x = x;
	startPos.y = y;

	// Load Textures form files
	questionTexture.loadFromFile(QUESTION_BLOCK);
	questionTexture.setSmooth(true);

	stoneTexture.loadFromFile(STONE_BLOCK);
	stoneTexture.setSmooth(true);

	for (int i = 0; i < 6; i++) {
		smashTextures[i].loadFromFile(SMASH_STONE_BLOCK + to_string(i) + ".png");
		smashTextures[i].setSmooth(true);
	}

	switch (type)
	{
	case QUESTION:
		blockSprite.setTexture(questionTexture);
		blockRect = questionRect;
		maxRect = 4;
		break;
	case STONE:
		blockSprite.setTexture(stoneTexture);
		blockSprite.setColor(Color::Color(70, 50, 180));
		blockRect = stoneRect;
		maxRect = 1;
		break;
	}

	// Set sprite properties
	blockSprite.setOrigin(blockRect.width / 2, blockRect.height / 2);
	blockSprite.setTextureRect(blockRect);
	blockSprite.setPosition(x, y);
	blockSprite.setScale(2, 2);

	// Set Hit Sound Properties
	popUpBuffer.loadFromFile(POPUP_SOUND);
	smashBuffer.loadFromFile(SMASH_SOUND);
	hitSound.setBuffer(popUpBuffer);
}


void Blocks::draw(RenderWindow& window) {
	if (display) {
		animation();
		window.draw(blockSprite);
	}
}


void Blocks::animation() {
	if (timer.getElapsedTime().asSeconds() > 0.2f) {
		switch (blockType) {
		case QUESTION:
			blockRect.left = questionRect.left + currentRect * questionRect.width;
			break;
		case BRONZE:
			blockSprite.setTexture(stoneTexture);
			blockRect = bronzeRect;
			maxRect = 1;
			break;
		case STONE:
			blockRect = stoneRect;
			break;
		case SMASH:
			if (!faid) {
				maxRect = 6;
				currentRect = 0;
				blockRect = smashRect;
				blockSprite.setOrigin(400, 400);
				blockSprite.setTexture(smashTextures[currentRect]);
				blockSprite.setScale(1, 1);
				faid = true;
			}
			else {
				blockSprite.setTexture(smashTextures[currentRect]);
				if (currentRect == maxRect - 1) display = false;
			}
		}

		blockSprite.setTextureRect(blockRect);
		currentRect++;

		if (currentRect == maxRect) currentRect = 0;

		timer.restart();
	}
	popUp();
	if(!faid) checkIntersection();
}

void Blocks::smash() {
	blockType = SMASH;
	hitSound.setBuffer(smashBuffer);
	hitSound.play();
}

void Blocks::startPopUp() {
	if (!isPopUp) {
		isPopUp = true;
		popUpTimer.restart();
		hitSound.play();
	}
}


void Blocks::popUp() {
	if (isPopUp) {
		int currentTime = popUpTimer.getElapsedTime().asMilliseconds();

		if (currentTime < 150) // GoingUp Time
		{
			movingSpeed += -1;
		}
		else if (currentTime < 200) // StandStill time
		{
			movingSpeed = 0;
		}
		else if (currentTime < 350) // GoingDown Time
		{
			movingSpeed += 1;
		}
		else 
		{
			if (blockType == QUESTION) blockType = BRONZE;

			movingSpeed = 0;
			isPopUp = false;
			blockSprite.setPosition(startPos.x, startPos.y);
			popUpTimer.restart();
		}
		blockSprite.move(0, movingSpeed);
	}
}


void Blocks::checkIntersection() {
	// Calculate Mario and Block bounds
	FloatRect marioBounds = mario->marioSprite.getGlobalBounds(),
		blockBounds = blockSprite.getGlobalBounds();
	Vector2f marioPos = mario->marioSprite.getPosition(), blockPos = blockSprite.getPosition();

	float marioCenterPointY = marioPos.y - (marioBounds.height / 2),
		blockTopPoint = blockPos.y - (blockBounds.height / 2),
		blockBottomPoint = blockPos.y + (blockBounds.height / 2),
		blockRightPoint = blockPos.x + (blockBounds.width / 2),
		blockLeftPoint = blockPos.x - (blockBounds.width / 2);
	
	// Handle large size of mario
	if (mario->marioState != SMALL) marioCenterPointY += 28;

	// In the block bounds
	if (blockBounds.intersects(marioBounds)) {
		if (marioPos.x >= blockLeftPoint && marioPos.x <= blockRightPoint) {
			if (mario->speed[1] > 0 && blockType != SMASH) { // jump on the block
				mario->marioSprite.setPosition(marioPos.x, blockBounds.top);
				mario->onGround = true;
				marioOn = true;
			}
			else if (mario->speed[1] < 0) { // Hit the block with head
				float blockBottom = blockBounds.top + blockBounds.height;

				// Handle large size of smash sprite
				if (blockType == SMASH) blockBottom = (blockBounds.top + blockBottom) / 2;

				mario->marioSprite.setPosition(marioPos.x, blockBottom + marioBounds.height);
				mario->speed[1] = 2;
				handleHitBlock();
			}
		}
		else { // touch from side
			if (marioCenterPointY > blockTopPoint + 8 && marioCenterPointY < blockBottomPoint - 8) { 
				float blockRight = blockBounds.left + blockBounds.width;
				if (marioPos.x > blockPos.x)
					mario->marioSprite.setPosition(blockRight + (marioBounds.width / 2), marioPos.y);
				else
					mario->marioSprite.setPosition(blockBounds.left - (marioBounds.width / 2), marioPos.y);
				mario->speed[0] = 0;
				mario->stuck = true;
			}
		}
	}
	else {
		if (marioOn && mario->onGround) { // Fall when mario left the block
			marioOn = false;
			mario->onGround = false;
			mario->speed[1] = -5;
		}

		// Fix Screen vibration when mario touch block side
		if (!mario->onGround) mario->stuck = false; 
		if (mario->stuck) {
			stuckRange = (marioPos.x > blockRightPoint) ? 230 : 100;
			if (abs(mario->marioSprite.getPosition().x - blockRightPoint) > stuckRange) {
				mario->stuck = false; // not touching the side anymore
			}
		}
	}

	
}


void Blocks::handleHitBlock() {
	switch (blockType)
	{
	case STONE:
		switch (mario->marioState) {
		case SMALL:
			startPopUp();
			break;
		case BIG:
		case SUPER:
			smash();
			break;
		}
		break;
	case QUESTION:
		startPopUp();
		break;
	default:
		break;
	}
}